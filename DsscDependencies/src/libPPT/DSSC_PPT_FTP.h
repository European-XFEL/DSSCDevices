/**
 * @file   DSSC_PPT_FTP.h
 * @Author Manfred Kirchgessner (Manfred.Kirchgessner@ziti.uni-heidelberg.de)
 * @date   August 2015
 * @brief  DSSC_PPT_FTP file transfer control class.
 *
 * Controls the file transfer between the Karabo or PPT_MAIN_GUI software and the
 * SlowControlServer running on the PPT. The linux native sockets are implemented.
 *
 */

#ifndef DSSCPPTFTP_HH
#define	DSSCPPTFTP_HH

#include <boost/shared_ptr.hpp>
#include <string>


namespace SuS {

  class DSSC_PPT_FTP {
  public:

    typedef boost::shared_ptr<DSSC_PPT_FTP> Pointer;

    /**
     * Standard Constructor of the class. Does nothing.*/
    DSSC_PPT_FTP()
    {
        ftpPort = 0;
        sockfd = 0;
        hostIPAddress = "not specified";
    }

    /**
     * Constructor of the class. Connects to the FTP server on the PPT.*/
    DSSC_PPT_FTP(std::string _hostIPAddress, int _ftpPort);

    /**
     * Destructor of the class. Closes the connection to the FTP server on the PPT.*/
    ~DSSC_PPT_FTP();

     /**
     * Sends a command to the FTP Server and waits for the answer.
     * @param buf send buffer that is filled with the characters of the cmd string
     * @param cmd command to send.*/
    void sendReadCommand(char * buf, std::string cmd);

    /**
     * Sends a command to the FTP Server and does not wait for the answer of the server.
     * @param buf send buffer that is filled with the characters of the cmd string
     * @param cmd command to send.
     * @see sendReadCommand()
     */
    void sendCommand(char * buf, std::string cmd);

    /**
     * Opens the connection the the FTP server on the PPT..
     * @param _hostIPAddress IP address string of the PPT.
     * @param _ftpPort ftp Port is 21.
     * @return true if connection could be established.
     */
    bool openFtp(std::string _hostIPAddress, int _ftpPort);

    /**
     * Send a file given by its path to the PPT. the file is stored in the
     * /tmp/ folder in the linux directory tree.
     * @param fileName file to send
     * @return true if file was sent successfully.
     */
    bool sendFile(std::string fileName);

    /**
     * Get a file given by its path from the PPT. The fileName must be relative to
     * the /tmp directory in the linux directory tree on the PPT.
     * @param fileName file to download
     * @return true if file was read successfully.
     */
    bool readFile(std::string fileName);

  private:
    std::string getBaseFileName(std::string filePathName);
    void error(const char *msg);
    int getOpenedPort(const char * buf);
    int getFileSize(const char * buf);
    int getReturnCode(const char * buf);

    int getOpenSocket(int dataPort);

  private:
    std::string hostIPAddress;
    int ftpPort;
    int sockfd;
  };
}


#endif	/* DSSCPPTFTP_HH */

