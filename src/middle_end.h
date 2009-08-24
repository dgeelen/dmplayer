#ifndef MIDDLE_END_H
	#define MIDDLE_END_H

	#include <vector>
	#include <boost/signal.hpp>
	#include <boost/thread.hpp>
	#include <boost/thread/mutex.hpp>
	#include <boost/strong_typedef.hpp>
	#include "packet.h"
	#include "synced_playlist.h"
	#include "network-handler.h"
	#include "util/thread_group2.h"
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
	 *       common configuration options.
	 * @todo Error handling is very poor atm (on error resume next...)
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
			 * @todo This still requires a lot of work to be implemented as support is needed from the networkhandler.
			 */
			boost::signal<void(const ipv4_socket_addr, ErrorID)> sig_connect_to_server_failure;

			/**
			 * Emitted when the client is disconnected by the server.
			 * The callback is provided with a string describing the reason for the disconnect.
			 * @note This signal is only raised upon receiving a message_disconnect, so not when the
			 *       client forcibly disconnects from a server.
			 */
			boost::signal<void(const std::string&)> sig_disconnected;

			/**
			 * Emitted when a track search completes. Initiate the search with search_tracks().
			 * @note The callback is provided with the SearchID that search_tracks() returned
			 *       and a list of tracks that matched the search parameters.
			 * @see search_tracks()
			 * @todo figure out if this callback may be executed more than once for the same SearchID
			 */
			boost::signal<void(const SearchID id, const std::vector<Track>&)> sig_search_tracks;

			/**
			 * Emitted when a local track search completes. Initiate the search with search_local_tracks().
			 * @note The callback is provided with the SearchID that search_local_tracks() returned
			 *       and a list of tracks that matched the search parameters.
			 * @see search_local_tracks()
			 * @todo figure out if this should return a list of LocalTrack or Track.
			 * @deprecated in favor of sig_search_tracks()
			 */
			boost::signal<void(const SearchID id, const std::vector<LocalTrack>&)> sig_search_local_tracks;

			/**
			 * Emitted when a global track search completes. Initiate the search with search_remote_tracks().
			 * @note The callback is provided with the SearchID that search_remote_tracks() returned
			 *       and a list of tracks that matched the search parameters.
			 * @see search_remote_tracks()
			 * @* @deprecated in favor of sig_search_tracks()
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

			/**
			 * Emitted when there is a change detected in the playlist.
			 * @note The client should return a reference to it's IPlaylist, so that
			 *       it's contents may be updated.
			 * @todo perhaps rename this function as it is also used to manipulate the playlist
			 *       in other ways. (for example to append tracks etc).
			 */
			boost::signal<IPlaylistRef(void)> sig_update_playlist;

			/**
			 * Emitted when the ClientID changes. For example when the client connects to a different server.
			 * @note The callback is provided with the new ClientID.
			 */
			boost::signal<void(ClientID)> sig_client_id_changed;

			/********************
			 ** Constructor(s) **
			 ********************/
			/**
			 * Constructor for middle_end.
			 * After instantiating an instance, connect all required signals then call connect_to_server().
			 * @see connect_to_server()
			 * @see ~middle_end()
			 */
			middle_end();

			/****************
			 ** Destructor **
			 ****************/
			/**
			 * Destructor for middle_end. Cleans up all resources allocated by the client and disconnects from any connected server.
			 * @see middle_end()
			 */
			~middle_end();

			/***************
			 ** Functions **
			 ***************/

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
			 * Aborts an ongoing connection attempt initiated by connect_to_server()
			 * @param address is the ipv4 address of the server which the client specified in the call to connect_to_server()
			 */
			void cancel_connect_to_server(const ipv4_socket_addr address);

			/**
			 * Initiates both a local and a global search.
			 * @note the seach is performed in a separate thead, which will call sig_search_tracks() upon
			 *       completion of the search.
			 * @param query contains the search parameters.
			 * @return a SearchID which can be used by the client to distinguish between different searches.
			 * @see sig_search_tracks()
			 * @todo decide wether to keep the local and global versions of search()
			 */
			SearchID search_tracks(const Track query);

			/**
			 * Initiates a search of the local track database.
			 * @note the seach is performed in a separate thead, which will call sig_search_local_tracks() upon
			 *       completion of the search.
			 * @param query contains the search parameters.
			 * @return a SearchID which can be used by the client to distinguish between different searches.
			 * @see sig_search_local_tracks()
			 * @deprecated in favor of search_tracks()
			 */
			SearchID search_local_tracks(const Track query);

			/**
			 * Initiates a global search for tracks.
			 * @note the seach is performed in a separate thead, which will call sig_search_remote_tracks() upon
			 *       completion of the search.
			 * @param query contains the search parameters.
			 * @return a SearchID which can be used by the client to distinguish between different searches.
			 * @see sig_search_remote_tracks()
			 * @deprecated in favor of search_tracks()
			 */
			SearchID search_remote_tracks(const Track query);



			/*********************************************
			 ** Operations on the client's own playlist **
			 *********************************************/

			/**
			 * Appends track to this client's playlist.
			 * @param track is a Track (obtain one from the TrackDataBase).
			 */
			void mylist_append(Track track);

			/**
			 * Appends every Track in tracklist to this client's playlist.
			 * @param tracklist is a std::vector of Track (obtain one from the TrackDataBase).
			 */
			void mylist_append(std::vector<Track> tracklist);

			/**
			 * Inserts track at position where in this client's playlist.
			 * @param track is a Track (obtain one from the TrackDataBase).
			 * @param where is an uint32 indicating the position in the client's playlist that track should be inserted.
			 */
			void mylist_insert(Track track, uint32 where);

			/**
			 * Inserts every Track in tracklist at position where in this client's playlist.
			 * @param tracklist is a std::vector of Track (obtain one from the TrackDataBase).
			 * @param where is an uint32 indicating the position in the client's playlist that tracklist should be inserted.
			 */
			void mylist_insert(std::vector<Track> tracklist, uint32 where);

			/**
			 * Removes track from the client's playlist.
			 * @param track is a Track.
			 */
			void mylist_remove(Track track);

			/**
			 * Removes every track in tracklist from the client's playlist.
			 * @param tracklist
			 */
			void mylist_remove(std::vector<Track> tracklist);

			/**
			 * Moves the track at position from in the client's playlist to position to.
			 * @param from a valid index into the playlist.
			 * @param to either a valid index into the playlist, or one past the last index to indicate an append.
			 */
			void mylist_reorder(uint32 from, uint32 to);

			/**
			 * Moves for each position <i>p</i> in fromlist, perform mylist_reorder(<i>p</i>, to), such that for each invocation of mylist_reorder() to is adjusted to reflect the index into the original playlist.
			 * @param fromlist a list of valid indices into the playlist.
			 * @param to either a valid index into the playlist, or one past the last index to indicate an append.
			 */
			void mylist_reorder(std::vector<uint32> fromlist, uint32 to);



			/**********************************************
			 ** Operations on the shared/global playlist **
			 **********************************************/

			/**
			 * Casts a positive vote for a track already in the (shared/global) playlist.
			 * @param track is the track that should be voted on.
			 */
			void playlist_vote_up(Track track);

			/**
			 * Casts a positive vote for tracks already in the (shared/global) playlist.
			 * @param tracklist is a list of Track containing tracks that should be voted on.
			 */
			void playlist_vote_up(std::vector<Track> track);

			/**
			 * Casts a negative vote for a track already in the (shared/global) playlist.
			 * @param track is the track that should be voted on.
			 */
			void playlist_vote_down(Track track);

			/**
			 * Casts a negative vote for tracks already in the (shared/global) playlist.
			 * @param tracklist is a list of Track containing tracks that should be voted on.
			 */
			void playlist_vote_down(std::vector<Track> track);

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
			 * @note get_known_servers() can be used in stead of sig_servers_added()/sig_servers_removed() to poll for servers.
			 *       This way it is possible to get a constantly updated reading of for example the Ping time of a server which
			 *       sig_servers_added()/sig_servers_removed() do not provide.
			 */
			std::vector<server_info> get_known_servers();

			/**
			 * Causes the list of known servers to be refreshed.
			 * Also invokes sig_servers_removed() for the current list, and sig_servers_added() for the fresh list.
			 */
			void refresh_server_list();

			/**
			 * @return the current ClientID.
			 * @todo Decide wether to keep this or remove it in favor of sig_connected_to_server and/or sig_client_id_changed
			 */
			ClientID get_client_id();

			/***************
			 ** Variables **
			 ***************/

		private: //NOTE: Private interfaces are documented in middle_end.cpp
			void                     handle_sig_server_list_update(const std::vector<server_info>&);
			void                     handle_received_message(const messageref m);
			boost::mutex             file_requests_mutex;
			typedef std::pair<std::pair<bool*, boost::thread::id>, LocalTrackID> file_requests_type;
			std::vector<file_requests_type> file_requests;
			void                     handle_message_request_file(const message_request_file_ref request, bool* done);
			void                     abort_all_file_transfers();
			void                     abort_file_transfer(LocalTrackID id);
			boost::mutex             search_mutex;
			void                     _search_tracks(SearchID search_id, const Track track);
			boost::mutex             known_servers_mutex;
			std::vector<server_info> known_servers;
			boost::mutex             client_id_mutex;
			ClientID                 client_id; // Valid *ONLY* when connected to a server (and vice-versa)
			util::thread_group2      threads;
			SearchID                 search_id;
			boost::mutex             search_id_mutex;
			network_handler          networkhandler;
			TrackDataBase            trackdb;
			boost::mutex             dest_server_mutex;
			ipv4_socket_addr         dest_server;
			SyncedPlaylist           client_synced_playlist;
	};

#endif //MIDDLE_END_H
