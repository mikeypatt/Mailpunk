#ifndef IMAP_H
#define IMAP_H
#include "imaputils.hpp"
#include <libetpan/libetpan.h>
#include <string>
#include <functional>

namespace IMAP {
    
class Session; // forward declaration to allow Message to access it


class Message {
    
    Session* session ;
    uint32_t uid ;
    
    //Header set to empty
    std::string body = "" ;
	std::string From = "" ;
	std::string Subject = "";
    
    
public:
	Message(Session* session,uint32_t uid) ;
	/**
	 * Get the body of the message. You may chose to either include the headers or not.
	 */
	std::string getBody();
	/**
	 * Get one of the descriptor fields (subject, from, ...)
	 */
	std::string getField(std::string fieldname);
	/**
	 * Remove this mail from its mailbox
	 */
	void deleteFromMailbox() ;
    
    //Helper function to extract the message items from the extracted attributes
    char* get_msg_att_msg_content(struct mailimap_msg_att * msg_att) ;
    
    //Helper function to extract the message attribute from the fetched list result
    char* get_msg_content(clist* fetch_result) ;
    
};



class Session {
        
    struct mailimap* imap ;
    Message** message = NULL ;
    uint32_t number_of_messages = 0;
    std::string curmailbox ;
    bool login_status = false ;
    std::function<void()> updateUI ;
    

 
public:
    
	Session(std::function<void()> updateUI);
    
	/**
	 * Get all messages in the INBOX mailbox terminated by a nullptr (like we did in class)
	 */
	Message** getMessages();
    
	/**
	 * connect to the specified server (143 is the standard unencrypte imap port)
	 */
	void connect(std::string const& server, size_t port = 143);

	/**
	 * log in to the server (connect first, then log in)
	 */
	void login(std::string const& userid, std::string const& password);

	/**
	 * select a mailbox (only one can be selected at any given time)
	 * 
	 * this can only be performed after login
	 */
	void selectMailbox(std::string const& mailbox);

    //Helper function to get the UID of the message
    uint32_t get_uid(struct mailimap_msg_att* msg_att) ;
    
    //Count the number of messages 
    uint32_t get_number_messages() ;

    
    friend Message ;
    
    ~Session();

    
    
};


}

#endif /* IMAP_H */
