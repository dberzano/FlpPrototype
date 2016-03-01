<?php
/// \file Socket.php
/// \brief Definition and implementation of the CERN\Alice\DAQ\O2\Socket class.
///
/// \author Adam Wegrzynek <adam.wegrzynek@cern.ch>
declare(strict_types=1);
namespace CERN\Alice\DAQ\O2;

/// Represents TCP-level logic of client connected to the server 
///
/// Provides basic functions to deal with TCP socket: read, write and close.
/// It is created for each connected client.
class Socket {
	/// stream resource that represent TCP socket of connected client
	private $socket;

	/// ID of socket
	protected $id;

	/// The orgin of the connection
	protected $hostname;

	/// Socket reference getter, needed by ServerSocket class to pass it to socket_select
	/// \return 	reference to socket
	public function &getSocket() 
	{
		return $this->socket;
	}
	
	/// Check if type of the socket is valid
	/// \return whether the socket type is type of stream or not
	public function isValid() 
	{
		if (get_resource_type($this->socket) === "stream") return true;
		return false;	
	}
	
	/// Socket class constructor, stores socket and initialize id and hostname variables
	/// \param 	reference to socket
	public function __construct(&$socket)
	{
		$this->socket  = $socket;
		$this->id = intval($this->socket);
		$this->hostname = stream_socket_get_name($this->socket, true);
		//stream_socket_enable_crypto($this->socket, true, STREAM_CRYPTO_METHOD_SSLv3_SERVER);
	}
	
	/// Writes the socket
	/// \param 	string that is written to socket
	/// \return number of written bytes
	/// \throw TcpException when failed to write the socket, or number of written bytes is 0
	protected function write(string $response): int
	{
		if (get_resource_type($this->socket) === "Unknown") {
			throw new TcpException(sprintf("Could not write socket because its type is Unknown : %s", substr($data, 0, 30)));
		}
		if (($bytes = stream_socket_sendto($this->socket, $response)) < 0) {
			throw new TcpException(sprintf("Could not write socket : %s", substr($response, 0, 30)));
		}
		Log::write(sprintf("%s <-- Sending %d bytes", $this->hostname, $bytes));
		return $bytes;
	}
	
	/// Reads the socket
	/// \param  number of bytes to read, default = 1500
	/// \return string that was read from the socket OR false when failed 	
	protected function read($toRead = 1500): string 
	{
		if (($return = stream_socket_recvfrom($this->socket, $toRead)) === false) {
			throw new TcpException(sprintf("%s, Error while reading %d bytes of data", $toRead, $this->hostname));
		}
		Log::write(sprintf("%s --> Reading %d bytes", $this->hostname, strlen($return)));
		return $return;
	}
	
	/// Closes to socket
	protected function close() 
	{
		stream_socket_shutdown($this->socket, STREAM_SHUT_RDWR);
		fclose($this->socket);
		Log::write(sprintf("Socket #%d has been closed.", $this->id));
	}
}
?>
