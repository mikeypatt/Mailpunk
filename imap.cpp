#include "imap.hpp"
#include "imaputils.hpp"
#include <iostream> 
#include <cstring>

using namespace IMAP ;

Session::Session(std::function<void()> updateUI):updateUI(updateUI) {
        //creates a new imap session called imap
        
        imap = mailimap_new(0, NULL);
        
}


void Session::connect(std::string const& server, size_t port)
{

    check_error(mailimap_socket_connect(imap ,server.c_str(),port),"sorry could not connect " ) ;
}
    
    
void Session::login(std::string const& userid, std::string const& password)
{
    //Log into the IMAP Session
    check_error(mailimap_login(imap,userid.c_str(),password.c_str()),"Sorry, could not log in to: ");
    login_status = true ;

}

void Session::selectMailbox(std::string const& mailbox)
{
    //Select the current mailbox specified in the UI
    curmailbox = mailbox ;
    check_error(mailimap_select(imap,mailbox.c_str()),"Sorry the mailbox could not be selected") ;
}


uint32_t Session::get_number_messages()
{
    
    clistiter* current_list_item;
     
    //Creates an empty list of status attributes 
    auto mailbox_status_list = mailimap_status_att_list_new_empty() ;
   
    // adds the number of messages in the mailbox 
    check_error(mailimap_status_att_list_add(mailbox_status_list,MAILIMAP_STATUS_ATT_MESSAGES),
                "Could not add the number of messages to the attribute list") ;
    
    mailimap_mailbox_data_status* status_list ;
    
    check_error(mailimap_status(imap,curmailbox.c_str(),mailbox_status_list,&status_list),
                            "Could not return the number of messsages status") ;
    
    auto status_info = (struct mailimap_status_info*) clist_content(clist_begin(status_list->st_info_list)) ;
    auto number_of_messages = status_info->st_value;
    
    //Free memory 
    mailimap_mailbox_data_status_free(status_list);
    mailimap_status_att_list_free(mailbox_status_list) ;
    
    return number_of_messages ;
}




Message** Session::getMessages()
{
    clist* fetched_uids  ;
    clistiter* current_uid ,current_msg_att ;
    
    //Prevent the entire function running upon Update UI
    if (message != NULL)
    {   
        return message ;
    }
    
    //Get number of messages
    number_of_messages = get_number_messages() ;
    if (number_of_messages == 0)
    {
        message = new Message*[1];
        message[0] = NULL;
        
        return message;
    }
    
    //creates the interval bounds to fetch the message
    auto interval_set = mailimap_set_new_interval(1,0);
    
    //creates a pointer to mailmap fetch attribute structure to request the unique indentifier of a message
    auto fetch_message_uid = mailimap_fetch_att_new_uid() ;
   
    
    //creates a pointer to mailimap fetch type structure 
    auto mailfetch_type = mailimap_fetch_type_new_fetch_att_list_empty() ;
    
    //Adds the fetch_message_uid attribute to the fetch type
    check_error(mailimap_fetch_type_new_fetch_att_list_add(mailfetch_type,fetch_message_uid),
                "Unable to add attributes to the fetch type") ;
               
    //Fetches the message
    check_error(mailimap_fetch(imap,interval_set,mailfetch_type,&fetched_uids),
                "Unable to fetch the message ID's from server") ;
            
    int i = 0 ;        
    
    //creates an array of message pointers 
    message = new Message* [number_of_messages+1]  ;      
      
    for(current_uid = clist_begin(fetched_uids) ; current_uid != NULL ; current_uid = clist_next(current_uid))
    {
        //get the list of attributes from the c_list of uids 
        auto msg_att = (struct mailimap_msg_att*) clist_content(current_uid);

        //iterate through the list of attributes
        uint32_t uid = get_uid(msg_att);
        if(uid != 0)
        {
            message[i] = new Message(this,uid) ;
            i++ ;
        }
    }
    //puts the end pointer to null
    message[i] = NULL ;
    
    //Free the memory
    mailimap_fetch_type_free(mailfetch_type) ;
    mailimap_fetch_list_free(fetched_uids) ;
    mailimap_set_free(interval_set) ;
    
    return message ;
}


uint32_t Session::get_uid(struct mailimap_msg_att* msg_att)
{
    clistiter* current_msg_att ;
	for(current_msg_att = clist_begin(msg_att->att_list) ; current_msg_att != NULL ; current_msg_att = clist_next(current_msg_att))
    {
		auto item = (mailimap_msg_att_item*) clist_content(current_msg_att);
        
        if(item->att_type == MAILIMAP_MSG_ATT_ITEM_STATIC && item->att_data.att_static-> att_type == MAILIMAP_MSG_ATT_UID)
        {
            return item->att_data.att_static->att_data.att_uid;  ;
        }
    }
    
    return 0 ;
	
}
   

Session::~Session()
{
    //Only delete if there are messages
    if(message != NULL)
    {
        for (int i = 0; message[i] != NULL; i++)
        {
            delete message[i];
        }
        delete [] message;
    }
    
    //Only logout if it is logged in
    if(login_status)
    {
        mailimap_logout(imap) ;
    }
    mailimap_free(imap) ;
}


Message::Message(Session* session,uint32_t uid): session(session) ,uid(uid){;
}

    
char* Message::get_msg_att_msg_content(struct mailimap_msg_att * msg_att)
{
	clistiter * cur;
	
	for(cur = clist_begin(msg_att->att_list) ; cur != NULL ; cur = clist_next(cur)) {
		
		
		auto item = (struct mailimap_msg_att_item*) clist_content(cur) ;
        
		if (item->att_type != MAILIMAP_MSG_ATT_ITEM_STATIC) {
			continue;
		}
		
    if (item->att_data.att_static->att_type != MAILIMAP_MSG_ATT_BODY_SECTION) {
			continue;
    }
		
		return item->att_data.att_static->att_data.att_body_section->sec_body_part;
	}
	
	return NULL;
}
    
char* Message::get_msg_content(clist* fetch_result)
{
	clistiter * cur;
	
 
	for(cur = clist_begin(fetch_result) ; cur != NULL ; cur = clist_next(cur)) {
		

		char * msg_content;
        
		auto msg_att = (mailimap_msg_att*) clist_content(cur) ;
        
		msg_content = get_msg_att_msg_content(msg_att);
        
		if (msg_content == NULL) {
			continue;
		}

		return msg_content;
	}
	
	return NULL;
}
    

std::string Message::getBody()
{

    if (this == NULL || session->number_of_messages == 0)
    {
        return "" ;
    }
    
    if(body != "")
    {
        return body ;
    }
    
    
    clist* fetched_body ;
    clistiter* att_iter ;
    char* msg_content ;
    
    //Creates a singular message set containing one message.
    auto single_msg_set = mailimap_set_new_single(uid);
    
    auto section = mailimap_section_new(NULL);
    auto body_att = mailimap_fetch_att_new_body_peek_section(section);
    
    auto message_fetch_type = mailimap_fetch_type_new_fetch_att_list_empty() ;
    
    check_error(mailimap_fetch_type_new_fetch_att_list_add(message_fetch_type ,body_att), "could not add the the body attribute") ;
    
    //Fetch Result
    check_error(mailimap_uid_fetch(session->imap,single_msg_set,message_fetch_type,&fetched_body) , "could not fetch the body of the message ") ;
    
    //Extract the fetched body from the list
    msg_content = get_msg_content(fetched_body);
    
    if (msg_content == NULL)
    {
        //Free memory
        mailimap_fetch_type_free(message_fetch_type) ; 
        mailimap_fetch_list_free(fetched_body);
        mailimap_set_free(single_msg_set);
        
        return "NO BODY";
    }
    
    //Assign message attribute
    body = msg_content ;
    
    //Free memory
    mailimap_fetch_type_free(message_fetch_type) ; 
    mailimap_fetch_list_free(fetched_body);
    mailimap_set_free(single_msg_set); 
    
    return body ;
    
}
    
std::string Message::getField(std::string fieldname)
{
    if (this == NULL || session->number_of_messages == 0)
    {
        return "" ;
    }
    
    std::string field;
    clist* fetched_result ;
 
    auto message_set = mailimap_set_new_single(uid) ;

    auto header_fetch = mailimap_fetch_type_new_fetch_att_list_empty();
    
    //I USED THIS FUNCTION AS IT SEEMED ALOT CLEANER AND DID NOT REQUIRE USE OF MALLOC
    auto header_section = mailimap_section_new_header() ;
  
    auto header_fetch_att = mailimap_fetch_att_new_body_section(header_section) ;
   
    check_error(mailimap_fetch_type_new_fetch_att_list_add(header_fetch,header_fetch_att),"Sorry,couldn't add header to attribute list") ;

    check_error(mailimap_uid_fetch(session->imap,message_set,header_fetch,&fetched_result),"Sorry,couldn't fetch the header");
    auto header_content = get_msg_content(fetched_result);
    
    if (header_content == NULL)
    {
        //Free memory
        mailimap_fetch_type_free(header_fetch) ; 
        mailimap_fetch_list_free(fetched_result);
        mailimap_set_free(message_set);
        
        return "NO FIELD";
    }

    // String parse to filter out the required section of the entire header
    field = header_content ;
    std::size_t field_start,field_end,result_start,result_end ;
    std::string temp = field ;
    std::string final_string ;

    field_start = temp.find(fieldname) ;
    field_end = temp.find(":",field_start+1) ;
   
    if(field_start != std::string::npos)
    {
        result_start = field_end + 2 ;
        result_end = temp.find("\n",result_start+1);
        final_string = temp.substr(result_start,result_end - result_start-1);

        if(fieldname == "Subject")
        {
            if(final_string == "" || final_string == "\n")
            {
                Subject = "No Subject" ;
                final_string = "No Subject" ;
            }else{
                Subject = final_string ;
            }
        }
        if(fieldname == "From")
        {
            if(final_string == "" || final_string == "\n")
            {
                Subject = "No Sender" ;
                final_string = "No Sender" ;
            }else{
                From = final_string ;
                
            }
            
        }
    }

    mailimap_fetch_type_free(header_fetch);
    mailimap_fetch_list_free(fetched_result) ;
    mailimap_set_free(message_set) ;
    
    
    return final_string ;
    
}
    

void Message::deleteFromMailbox()
{
    if (this == NULL || session->number_of_messages == 0)
    {
        return ;
    }

    auto delete_flag_list = mailimap_flag_list_new_empty();

    auto delete_flag = mailimap_flag_new_deleted();
    
    check_error(mailimap_flag_list_add(delete_flag_list,delete_flag),"Unable to add delete flag to message");
    
    auto store_att_flags = mailimap_store_att_flags_new_set_flags(delete_flag_list);
    
    auto message_set = mailimap_set_new_single(uid);
    
    check_error(mailimap_uid_store(session->imap,message_set,store_att_flags),"Unable to update selected message flag");

    check_error(mailimap_expunge(session->imap),"unable to delete the message") ;

    int i = 0 ;
    while(session->message[i] != NULL  && session->message[i] != this)
    {
 
        i++ ;
    }
    while(i<session->number_of_messages)
    {
        session->message[i] = session->message[i+1] ;
        i++ ;
    }
    
    mailimap_store_att_flags_free(store_att_flags); //Free memory
    mailimap_set_free(message_set); //Free memory
    session->updateUI() ;

    delete this ;
    

}














