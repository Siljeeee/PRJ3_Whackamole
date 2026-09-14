#include <iostream>
#include <algorithm>
#include <restinio/all.hpp>
#include <json_dto/pub.hpp>

struct place_t
{
	place_t() = default;

	place_t(std::string placename, double lat, double lon)
		: m_placename{ std::move(placename) }
		, m_lat{ lat }
		, m_lon{ lon }
	{}

	template < typename JSON_IO >
	void json_io(JSON_IO & io)
	{
		io
			& json_dto::mandatory("PlaceName", m_placename)
			& json_dto::mandatory("Lat", m_lat)
			& json_dto::mandatory("Lon", m_lon);
	}

	std::string m_placename;
	double m_lat{};
	double m_lon{};
};

struct weatherStation_t
{
	weatherStation_t() = default;

	weatherStation_t(int id, std::string date, std::string time,
		place_t place, double temperature, double humidity)
		: m_id{ id }
		, m_date{ std::move(date) }
		, m_time{ std::move(time) }
		, m_place{ std::move(place) }
		, m_temperature{ temperature }
		, m_humidity{ humidity }
	{}

	template < typename JSON_IO >
	void json_io(JSON_IO & io)
	{
		io
			& json_dto::mandatory("ID", m_id)
			& json_dto::mandatory("Date", m_date)
			& json_dto::mandatory("Time", m_time)
			& json_dto::mandatory("Place", m_place)
			& json_dto::mandatory("Temperature", m_temperature)
			& json_dto::mandatory("Humidity", m_humidity);
	}

	int m_id{};
	std::string m_date;
	std::string m_time;
	place_t m_place;
	double m_temperature{};
	double m_humidity{};
};

using weather_collection_t = std::vector< weatherStation_t >;

namespace rr = restinio::router;
using router_t = rr::express_router_t<>;

class weather_handler_t
{
public:
	explicit weather_handler_t(weather_collection_t & stations)
		: m_stations(stations)
	{}

	// GET /weather — return last 3 registrations
	auto on_get_latest(const restinio::request_handle_t& req, rr::route_params_t) const
	{
		auto resp = init_resp(req->create_response());

		weather_collection_t latest;
		auto start = m_stations.size() > 3 ? m_stations.size() - 3 : 0;
		for(std::size_t i = start; i < m_stations.size(); ++i)
			latest.push_back(m_stations[i]);

		resp.set_body(json_dto::to_json(latest));
		return resp.done();
	}

	// GET /weather/date/:date — return registrations for a specific date
	auto on_get_by_date(const restinio::request_handle_t& req, rr::route_params_t params) const
	{
		auto resp = init_resp(req->create_response());
		const auto date = restinio::cast_to<std::string>(params["date"]);

		weather_collection_t result;
		for(const auto & s : m_stations)
			if(s.m_date == date)
				result.push_back(s);

		if(result.empty())
		{
			resp.set_body("{\"error\":\"No data for date: " + date + "\"}");
			return resp.done();
		}

		resp.set_body(json_dto::to_json(result));
		return resp.done();
	}

	// GET /weather/:id — return registration with specific ID
	auto on_get_by_id(const restinio::request_handle_t& req, rr::route_params_t params) const
	{
		auto resp = init_resp(req->create_response());
		const auto id = restinio::cast_to<int>(params["id"]);

		for(const auto & s : m_stations)
		{
			if(s.m_id == id)
			{
				resp.set_body(json_dto::to_json(s));
				return resp.done();
			}
		}

		resp.set_body("{\"error\":\"ID not found: " + std::to_string(id) + "\"}");
		return resp.done();
	}

	// PUT /weather/:id — update existing registration
	auto on_put_weather(const restinio::request_handle_t& req, rr::route_params_t params)
	{
		auto resp = init_resp(req->create_response());
		const auto id = restinio::cast_to<int>(params["id"]);

		try
		{
			auto updated = json_dto::from_json<weatherStation_t>(req->body());
			for(auto & s : m_stations)
			{
				if(s.m_id == id)
				{
					updated.m_id = id;
					s = updated;
					resp.set_body(json_dto::to_json(s));
					return resp.done();
				}
			}
			resp.set_body("{\"error\":\"ID not found: " + std::to_string(id) + "\"}");
		}
		catch(const std::exception & ex)
		{
			resp.set_body("{\"error\":\"Invalid JSON: " + std::string(ex.what()) + "\"}");
		}

		return resp.done();
	}

	// POST /weather — add new registration
	auto on_post_weather(const restinio::request_handle_t& req, rr::route_params_t)
	{
		auto resp = init_resp(req->create_response());

		try
		{
			auto station = json_dto::from_json<weatherStation_t>(req->body());
			station.m_id = static_cast<int>(m_stations.size()) + 1;
			m_stations.push_back(station);

			resp.set_body(json_dto::to_json(station));
		}
		catch(const std::exception & ex)
		{
			resp.set_body("{\"error\":\"Invalid JSON: " + std::string(ex.what()) + "\"}");
		}

		return resp.done();
	}

	// OPTIONS handler for CORS preflight
	auto options(restinio::request_handle_t req, rr::route_params_t)
	{
		auto resp = init_resp(req->create_response());
		resp.append_header("Access-Control-Allow-Methods", "OPTIONS, GET, POST, PUT");
		resp.append_header("Access-Control-Allow-Headers", "content-type");
		resp.append_header("Access-Control-Max-Age", "86400");
		return resp.done();
	}

private:
	weather_collection_t & m_stations;

	template < typename RESP >
	static RESP init_resp(RESP resp)
	{
		resp
			.append_header("Server", "RESTinio sample server /v.0.6")
			.append_header_date_field()
			.append_header("Content-Type", "application/json; charset=utf-8") // lab3: added application/json
			.append_header(restinio::http_field::access_control_allow_origin, "*");
		return resp;
	}
};

auto server_handler(weather_collection_t & stations)
{
	auto router = std::make_unique<router_t>();
	auto handler = std::make_shared<weather_handler_t>(std::ref(stations));

	auto by = [&](auto method) {
		using namespace std::placeholders;
		return std::bind(method, handler, _1, _2);
	};

	// NOTE: /weather/date/:date must be registered before /weather/:id
	router->http_get("/weather", by(&weather_handler_t::on_get_latest));
	router->http_get("/weather/date/:date", by(&weather_handler_t::on_get_by_date));
	router->http_get(R"(/weather/:id(\d+))", by(&weather_handler_t::on_get_by_id));
	router->http_post("/weather", by(&weather_handler_t::on_post_weather));
	router->http_put(R"(/weather/:id(\d+))", by(&weather_handler_t::on_put_weather));

	// Lab 3: OPTIONS routes for CORS preflight on each endpoint
	router->add_handler(restinio::http_method_options(), "/weather", by(&weather_handler_t::options));
	router->add_handler(restinio::http_method_options(), "/weather/date/:date", by(&weather_handler_t::options));
	router->add_handler(restinio::http_method_options(), R"(/weather/:id(\d+))", by(&weather_handler_t::options));

	return router;
}

int main()
{
	using namespace std::chrono;

	try
	{
		using traits_t = restinio::traits_t<
			restinio::asio_timer_manager_t,
			restinio::single_threaded_ostream_logger_t,
			router_t >;

		weather_collection_t stations{
			{ 1, "2024-04-20", "08:00", { "Aarhus", 56.1629, 10.2039 }, 12.1, 60.0 },
			{ 2, "2024-04-20", "12:00", { "Aarhus", 56.1629, 10.2039 }, 15.3, 62.0 },
			{ 3, "2024-04-20", "16:00", { "Aarhus", 56.1629, 10.2039 }, 14.8, 65.0 },
			{ 4, "2024-04-21", "08:00", { "Copenhagen", 55.6761, 12.5683 }, 11.0, 70.0 }
		};

		restinio::run(
			restinio::on_this_thread<traits_t>()
				.address("0.0.0.0")
				.port(8080)
				.request_handler(server_handler(stations))
				.read_next_http_message_timelimit(10s)
				.write_http_response_timelimit(1s)
				.handle_request_timeout(1s));
	}
	catch(const std::exception & ex)
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}
