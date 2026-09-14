/*
 * KNP module 2 Sample starter code using Restinio library (HTTP with C++) for Client-Server
 * and you can also use this in your PRJ3 project
 * Underviser: "Jenny" Jung Min Kim
 *
 * Purpose:
 *   Minimal example showing how to use the Restinio library to handle HTTP protocol and
 *   HTTP-based Client–Server communication. 
 *   (You’ll learn HTTP and implementations in detail in November in the KNP course.)
 *
 * IMPORTANT — Adapt for your PRJ3 project:
 *   ( You will learn these steps in KNP module 2 gradually)
 *   1) Replace book_t/book_collection/book_collection_t/book_handler_t
 *      with your own data structure, collections, and handlers. 
 *   2) Update the initial values in main() to match your data and logic.
 *   3) Add the HTTP endpoints you need (GET/POST/PUT/DELETE, etc.).
 */

#include <iostream>
#include <restinio/all.hpp>
#include <json_dto/pub.hpp>

//=========================*/
// Example data structure
// TODO later: Replace 'book_t' with your own struct (e.g. something_t).
//=========================*/
struct book_t  // This is Data structure (sample code has book_t, you need to define your own data struct)
{
	book_t() = default;

	book_t(std::string author, std::string title)
		: m_author{ std::move(author) }, m_title{ std::move(title) }
	{}

	template < typename JSON_IO >
	void json_io(JSON_IO & io)
	{
		io
			& json_dto::mandatory("author", m_author)
			& json_dto::mandatory("title", m_title);
	}

	std::string m_author;
	std::string m_title;
};

//=========================*/
// Todo later: Change this alias for your project type, e.g. vector<something_t>
//=========================*/
using book_collection_t = std::vector< book_t >;

namespace rr = restinio::router;
using router_t = rr::express_router_t<>;

//=========================*/
// HTTP handler class
// TODO later: Rename/modify methods for your data model
//=========================*/
class books_handler_t
{
public:
	explicit books_handler_t(book_collection_t & books)
		: m_books(books)
	{}

	auto on_books_list(const restinio::request_handle_t& req, rr::route_params_t) const
	{
		auto resp = init_resp(req->create_response());

		resp.set_body("Book collection (book count: " +
			std::to_string(m_books.size()) + ")\n");

		for(std::size_t i = 0; i < m_books.size(); ++i)
		{
			resp.append_body(std::to_string(i + 1) + ". ");
			const auto & b = m_books[i];
			resp.append_body(b.m_title + " [" + b.m_author + "]\n");
		}

		return resp.done();
	}

private:
	book_collection_t & m_books;  // TODO: Replace 'book_collection_t', m_books with your own ..

	template < typename RESP >
	static RESP init_resp(RESP resp)
	{
		resp
			.append_header("Server", "RESTinio sample server /v.0.6")
			.append_header_date_field()
			.append_header("Content-Type", "text/plain; charset=utf-8");
		return resp;
	}

	static void mark_as_bad_request(auto & resp)  // (auto & resp) with C++20 , if C++17, it should be (RESP & resp)
	{
		resp.header().status_line(restinio::status_bad_request());
	}
};

//=========================*/
// Router setup
// TODO later: Add new endpoints for your project (HTTP POST, PUT, etc.) - we will learn gradually in lecture KNP module2
//=========================*/
auto server_handler(book_collection_t & book_collection) //replace book_ .. 
{
	auto router = std::make_unique<router_t>();
	auto handler = std::make_shared<books_handler_t>(std::ref(book_collection));

	auto by = [&](auto method) {
		using namespace std::placeholders;
		return std::bind(method, handler, _1, _2);
	};

	// Example: GET /
	router->http_get("/", by(&books_handler_t::on_books_list));  //replace book_ .. 

	return router;
}

//=========================*/
// Main entry point
//=========================*/
int main()
{
	using namespace std::chrono;

	try
	{
		using traits_t = restinio::traits_t<
			restinio::asio_timer_manager_t,
			restinio::single_threaded_ostream_logger_t,
			router_t >;

		//=========================*/
		// TODO later: Replace this hardcoded collection with your own initial values
		//=========================*/
		book_collection_t book_collection{
			{"Agatha Christie", "Murder on the Orient Express"},
			{"Agatha Christie", "Sleeping Murder"},
			{"B. Stroustrup", "The C++ Programming Language"}
		};

		//=========================*/
		// Run server
		// IMPORTANT:
		//   - "0.0.0.0" means server listens on ALL network interfaces.
		//   - Use Pi's IP + port 8080 to connect from another device.
		//=========================*/
		restinio::run(
			restinio::on_this_thread<traits_t>()
				.address("0.0.0.0")   // For Pi: allow access from outside
				.port(8080)           // Default port, change if needed
				.request_handler(server_handler(book_collection))
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
