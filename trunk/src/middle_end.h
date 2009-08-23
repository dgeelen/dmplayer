#ifndef MIDDLE_END_H
	#define MIDDLE_END_H

	#include <vector>
	#include <boost/signal.hpp>
	#include <boost/strong_typedef.hpp>
	#include "packet.h"
	#include "network-handler.h"
	#include "playlist_management.h"
	#include "network/network-core.h"

	BOOST_STRONG_TYPEDEF(uint32, SearchID);
	BOOST_STRONG_TYPEDEF(uint32, ErrorID);

	/**
	 * The purpose of the middle end is to provide a convenient wrapper for
	 * all client related functionality, in order to facilitate the rapid
	 * development of new clients.
	 *
	 * @todo this class should provide a way for clients to store/retrieve
	 *       @common configuration options.
	 */
	class middle_end {
		public:
			/*************
			 ** Signals **
			 *************/
			/**
			 * Emitted when a connection is established with a server. Initiate the connection with connect_to_server()
			 * @note The callback is provided with the ipv4_socket_addr that was supplied to connect_to_server()
			 *       and the ClientID that the server provided.
			 * @see connect_to_server()
			 */
			boost::signal<void(const ipv4_socket_addr, ClientID)> sig_connect_to_server_success;

			/**
			 * Emitted when a connection with a server could not be established.
			 * Initiate the connection with connect_to_server().
			 * @note The callback is provided with the ipv4_socket_addr that was supplied to connect_to_server()
			 *       and an error code indicating the reason the connection could not be established.
			 * @see connect_to_server()
			 */
			boost::signal<void(const ipv4_socket_addr, ErrorID)> sig_connect_to_server_failure;

			/**
			 * Emitted when a local track search completes. Initiate the search with search_local_tracks().
			 * @note The callback is provided with the SearchID that search_local_tracks() returned
			 *       and a list of tracks that matched the search parameters.
			 * @see search_local_tracks()
			 */
			boost::signal<void(const SearchID id, const std::vector<Track>&)> sig_search_local_tracks;

			/**
			 * Emitted when a global track search completes. Initiate the search with search_remote_tracks().
			 * @note The callback is provided with the SearchID that search_remote_tracks() returned
			 *       and a list of tracks that matched the search parameters.
			 * @see search_remote_tracks()
			 */
			boost::signal<void(const SearchID id, const std::vector<Track>&)> sig_search_remote_tracks;

			/**
			 * Emitted when a message from the network is received.
			 * @todo Decide wether all messages or only unhandled messages are passed to the client application.
			 */
			boost::signal<void(const messageref)> sig_receive_message;

			/**
			 * Emitted when new servers are detected on the network.
			 * @note The callback is provided with a list which contains the servers which
			 *       were not previously detected.
			 */
			boost::signal<void(const std::vector<server_info>&)> sig_servers_added;

			/**
			 * Emitted when servers are detected missing on the network.
			 * @note The callback is provided with a list which contains the servers which
			 *       are no longer present.
			 */
			boost::signal<void(const std::vector<server_info>&)> sig_servers_removed;



			/********************
			 ** Constructor(s) **
			 ********************/
			/**
			 * Constructor for middle_end.
			 * After instantiating an instance, connect all required signals then call start().
			 * @see start()
			 */
			middle_end();

			/***************
			 ** Functions **
			 ***************/
			/**
			 * When a client has connected all signals it is interested in and is ready to begin
			 * receiving events it should call start() to notify the middle_end.
			 * @see middle_end()
			 */
			void start();

			/**
			 * Instructs the middle_end to attempt to connect to the server present on address
			 * @param address is the ipv4 address of the server which the client wishes to connect with.
			 * @param timeout is the amount of time (in seconds) before giving up. A value of 0 indicates no timeout.
			 * @see sig_connect_to_server_succes()
			 * @see sig_connect_to_server_failure()
			 * @todo Timeout is currently not honoured due to lack of support in the network-handler.
			 */
			void connect_to_server(const ipv4_socket_addr address, int timeout = 0);

			/**
			 * Initiates a search of the local track database.
			 * @note the seach is performed in a separate thead, which will call sig_search_local_tracks() upon
			 *       completion of the search.
			 * @param track contains the search parameters.
			 * @return a SearchID which can be used by the client to distinguish between different searches.
			 * @see sig_search_local_tracks()
			 */
			SearchID search_local_tracks(const Track track);

			/**
			 * Initiates a global search for tracks.
			 * @note the seach is performed in a separate thead, which will call sig_search_remote_tracks() upon
			 *       completion of the search.
			 * @param track contains the search parameters.
			 * @return a SearchID which can be used by the client to distinguish between different searches.
			 * @see sig_search_remote_tracks()
			 */
			SearchID search_remote_tracks(const Track track);

			/**
			 * @return a descriptive message for a given ErrorID
			 * @param errorid the ErrorID for which the description is desired.
			 */
			std::string get_error_description(ErrorID errorid);

			/**
			 * Sends a message to the server.
			 * @param mr is a messageref to the message to be send.
			 */
			void send_message(const messageref mr);

			/**
			 * @return the list of currently known servers.
			 * @todo Decide wether to keep this or remove it in favor of sig_servers_added/sig_servers_deleted
			 */
			std::vector<server_info> get_known_servers();

			/**
			 * @return the current ClientID.
			 * @todo Decide wether to keep this or remove it in favor of sig_connected_to_server
			 */
			ClientID get_client_id();

			/***************
			 ** Variables **
			 ***************/
			/**
			 * The TrackDataBase which stores this clients tracks.
			 * @todo this should probably be hidden behind a set of convenience functions which
			 *       handle things such as which directories and filetypes to add to the database
			 *       as well as other settings.
			 */
			TrackDataBase trackdb;

		private: //NOTE: Private interfaces are documented in middle_end.cpp
			void                     handle_sig_server_list_update(const std::vector<server_info>&);
			void                     handle_received_message(const messageref m);
			std::vector<server_info> known_servers;
			ClientID                 client_id;
			network_handler          networkhandler;
			ipv4_socket_addr         dest_server;
	};

#endif //MIDDLE_END_H
