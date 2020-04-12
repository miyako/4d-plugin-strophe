/* --------------------------------------------------------------------------------
 #
 #  4DPlugin-strophe.cpp
 #	source generated by 4D Plugin Wizard
 #	Project : strophe
 #	author : miyako
 #	2019/10/11
 #  
 # --------------------------------------------------------------------------------*/

#include "4DPlugin-strophe.h"

void OnStartup() {
    xmpp_initialize();
}

void OnExit() {
    xmpp_shutdown();
}

#pragma mark -

void PluginMain(PA_long32 selector, PA_PluginParameters params) {
    
	try
	{
        switch(selector)
        {
            case kInitPlugin :
            case kServerInitPlugin :
                OnStartup();
                break;
                
            case kDeinitPlugin :
            case kServerDeinitPlugin :
                OnExit();
                break;
                
			// --- strophe
            
            case 1 :
                xmpp_send(params, xmpp_stanza_type_message);
                break;
            case 2 :
                xmpp_send(params, xmpp_stanza_type_connect);
                break;
        }

	}
	catch(...)
	{

	}
}

#pragma mark -

typedef struct {
    
    const char *stanza;
    xmpp_ctx_t *ctx;
    PA_CollectionRef collection;
    bool got_message;
    
} user_ctx_t;

void log_handler(void * const userdata,
      const xmpp_log_level_t level,
      const char * const area,
      const char * const msg) {
    
    PA_CollectionRef c = (PA_CollectionRef)userdata;
    
    PA_ObjectRef o = PA_CreateObject();
    ob_set_s(o, L"area", area);
    ob_set_s(o, L"message", msg);
    
    PA_Variable v = PA_CreateVariable(eVK_Object);
    PA_SetObjectVariable(&v, o);
    
    PA_SetCollectionElement(c, PA_GetCollectionLength(c), v);
}

#pragma mark stanza

int version_handler(xmpp_conn_t * const conn,
                    xmpp_stanza_t * const stanza,
                    void * const userdata) {
    
    xmpp_stanza_t *reply, *query, *name, *version, *text;
    const char *ns;
    xmpp_ctx_t *ctx = (xmpp_ctx_t*)userdata;

//    printf("Received version request from %s\n", xmpp_stanza_get_from(stanza));

    reply = xmpp_stanza_reply(stanza);
    xmpp_stanza_set_type(reply, "result");

    query = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(query, "query");
    ns = xmpp_stanza_get_ns(xmpp_stanza_get_children(stanza));
    if (ns) {
        xmpp_stanza_set_ns(query, ns);
    }

    name = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(name, "name");
    xmpp_stanza_add_child(query, name);
    xmpp_stanza_release(name);

    text = xmpp_stanza_new(ctx);
    xmpp_stanza_set_text(text, "libstrophe");
    xmpp_stanza_add_child(name, text);
    xmpp_stanza_release(text);

    version = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(version, "version");
    xmpp_stanza_add_child(query, version);
    xmpp_stanza_release(version);

    text = xmpp_stanza_new(ctx);
    xmpp_stanza_set_text(text, "0.9.3");
    xmpp_stanza_add_child(version, text);
    xmpp_stanza_release(text);

    xmpp_stanza_add_child(reply, query);
    xmpp_stanza_release(query);

    xmpp_send(conn, reply);
    xmpp_stanza_release(reply);

    return 1;/* do not remove, we will release it with the connection */
}

int message_handler(xmpp_conn_t * const conn, xmpp_stanza_t * const stanza, void * const userdata) {
    
    user_ctx_t *mem = (user_ctx_t *)userdata;
    PA_CollectionRef c = (PA_CollectionRef)mem->collection;
     
    mem->got_message = true;
    
    xmpp_stanza_t *body = xmpp_stanza_get_child_by_name(stanza, "body");
    
    if (body == NULL) {
        /* ignore message with no body */
        return 1;/* do not remove, we will release it with the connection */
    }
        
    const char *type = xmpp_stanza_get_type(stanza);
    if (type == NULL) {
        /* ignore message with no type */
        return 1;/* do not remove, we will release it with the connection */
    }
        
    PA_ObjectRef o = PA_CreateObject();
    
    char *text = xmpp_stanza_get_text(body);
    const char *name = xmpp_stanza_get_name(stanza);
    const char *from = xmpp_stanza_get_from(stanza);
    const char *to = xmpp_stanza_get_to(stanza);
    const char *mid = xmpp_stanza_get_id(stanza);
    const char *ns = xmpp_stanza_get_ns(stanza);

    ob_set_s(o, L"body", text);
    ob_set_s(o, L"type", type);
    ob_set_s(o, L"name", name);
    ob_set_s(o, L"from", from);
    ob_set_s(o, L"to", to);
    ob_set_s(o, L"id", mid);
    ob_set_s(o, L"ns", ns);
    
    if(text) {
        free(text);
    }
      
    PA_Variable v = PA_CreateVariable(eVK_Object);
    PA_SetObjectVariable(&v, o);
    
    PA_SetCollectionElement(c, PA_GetCollectionLength(c), v);
    
    /* disconnect in timed_handler */
        
    return 1;/* do not remove, we will release it with the connection */
}

int timed_handler(xmpp_conn_t * const conn,
                    void * const userdata) {
    
    user_ctx_t *mem = (user_ctx_t *)userdata;

    if(mem->got_message) {
        mem->got_message = false;
    }else{
        xmpp_disconnect(conn);
    }

    return 1;/* do not remove */
}

#define TIMED_HANDLER_INTERVAL 100

void stanza_type_connect_handler(xmpp_conn_t * const conn,
                                 const xmpp_conn_event_t status,
                                 const int error,
                                 xmpp_stream_error_t * const stream_error,
                                 void * const userdata) {
    
    user_ctx_t *mem = (user_ctx_t *)userdata;
    xmpp_ctx_t *ctx = (xmpp_ctx_t *)mem->ctx;
    
    if (status == XMPP_CONN_CONNECT) {
        xmpp_handler_add(conn, version_handler, "jabber:iq:version", "iq", NULL, userdata);
        xmpp_handler_add(conn, message_handler, NULL, "message", NULL, userdata);
        
        xmpp_timed_handler_add    (conn,
                                   timed_handler,
                                   TIMED_HANDLER_INTERVAL,
                                   userdata);
        
        xmpp_stanza_t *pres = xmpp_presence_new(ctx);
        xmpp_send(conn, pres);
        xmpp_stanza_release(pres);
        
    }
    else {
        /* disconnected */
        xmpp_stop(ctx);
    }
}

void stanza_type_message_handler(xmpp_conn_t * const conn,
                                 const xmpp_conn_event_t status,
                                 const int error,
                                 xmpp_stream_error_t * const stream_error,
                                 void * const userdata) {
    
    user_ctx_t *mem = (user_ctx_t *)userdata;
    xmpp_ctx_t *ctx = (xmpp_ctx_t *)mem->ctx;
    
    if (status == XMPP_CONN_CONNECT) {
        
        Json::Value root;
        Json::CharReaderBuilder builder;
        std::string errors;

        Json::CharReader *reader = builder.newCharReader();
        bool parse = reader->parse(mem->stanza,
                                   mem->stanza + strlen(mem->stanza),
                                   &root,
                                   &errors);
        if(parse)
        {
            if(root.isObject())
            {
                xmpp_stanza_t *message = xmpp_stanza_new(ctx);
                char *uuid = xmpp_uuid_gen(ctx);

                xmpp_stanza_set_type(message, "chat");
                xmpp_stanza_set_name(message, "message");
                xmpp_stanza_set_ns(message, "jabber:client");
                xmpp_stanza_set_from(message, xmpp_conn_get_jid(conn));
                xmpp_stanza_set_attribute(message, "xml:lang", "en");
                xmpp_stanza_set_id(message, uuid);
                xmpp_free(ctx, uuid);

                for(Json::Value::const_iterator it = root.begin() ; it != root.end() ; it++)
                {
                    if(it->isString())
                    {
                        JSONCPP_STRING name = it.name();
                        JSONCPP_STRING value = it->asString();
                        
                        if(name == "body"){
                            xmpp_stanza_t *body = xmpp_stanza_new(ctx);
                            xmpp_stanza_set_name(body, "body");
                            xmpp_stanza_t *body_data = xmpp_stanza_new(ctx);
                            xmpp_stanza_set_text(body_data, (const char *)value.c_str());
                            xmpp_stanza_add_child(message, body);
                            xmpp_stanza_add_child(body, body_data);
                            xmpp_stanza_release(body_data);
                            xmpp_stanza_release(body);
                        }else{
                            /* ns, id, type, to, from... */
                            xmpp_stanza_set_attribute(message,
                                                      (const char *)name.c_str(),
                                                      (const char *)value.c_str());
                        }
                    }
                }
                xmpp_send(conn, message);
                xmpp_stanza_release(message);
            }
            
        }
        xmpp_disconnect(conn);
    }
    else {
        /* disconnected */
        xmpp_stop(ctx);
    }
}

void xmpp_send(PA_PluginParameters params, xmpp_stanza_type_t type) {

    PA_ObjectRef status = PA_CreateObject();
    PA_ObjectRef options = NULL;
    PA_ObjectRef stanza = NULL;
    PA_CollectionRef messages = NULL;
        
    switch (type) {
        case xmpp_stanza_type_message:
            options = PA_GetObjectParameter(params, 1);
            stanza = PA_GetObjectParameter(params, 2);
            break;
        case xmpp_stanza_type_connect:
            options = PA_GetObjectParameter(params, 1);
            messages = PA_CreateCollection();
            break;
        default:
            break;
    }

    CUTF8String json_stanza, json_handers;
    
    if(stanza) {
        ob_stringify(stanza, &json_stanza);
    }
    
    user_ctx_t mem;
    mem.stanza = (const char *)json_stanza.c_str();
    mem.collection = messages;
    mem.got_message = false;
    
    xmpp_log_level_t log_level = XMPP_LEVEL_DEBUG;
    
    int run_timeout = 20;
    int keepalive_timeout = 60;
    int keepalive_interval = 1;
    bool keepAlive = false;
    
    bool enableLegacyAuth = false;
    
    bool disableTLS = false;
    bool mandatoryTLS = true;
    
    /* flags */
    bool legacyTLS = false; /* implicit SSL without starttls */
    bool trustTLS = true;  /* trust server's certificate even if it is invalid */
    // maybe: XMPP_CONN_FLAG_LEGACY_AUTH support
    
    CUTF8String jid, password, host;
    long flags = 0;
    
    if (options) {
        
        if(ob_is_defined(options, L"logLevel"))
        {
            int logLevel = (int)ob_get_n(options, L"logLevel");
            if((logLevel == -1)
               || (logLevel > XMPP_LEVEL_ERROR)
               || (logLevel < -1)) {
                logLevel = NULL;
            }
            log_level = (xmpp_log_level_t)logLevel;
        }

        if(ob_is_defined(options, L"keepAlive"))
        {
            keepAlive = ob_get_b(options, L"keepAlive");
        }
        
        if(ob_is_defined(options, L"keepAliveTimeout"))
        {
            int keepAliveTimeout = (int)ob_get_n(options, L"keepAliveTimeout");
            if(keepAliveTimeout > 0) {
                keepalive_timeout = keepAliveTimeout;
                keepAlive = true;
            }
        }
        
        if(ob_is_defined(options, L"keepAliveInterval"))
        {
            int keepAliveInterval = (int)ob_get_n(options, L"keepAliveInterval");
            if(keepAliveInterval > 0) {
                keepalive_interval = keepAliveInterval;
                keepAlive = true;
            }
        }

        if(ob_is_defined(options, L"timeout"))
        {
            int timeout = (int)ob_get_n(options, L"timeout");
            if(timeout > 0) {
                run_timeout = timeout;
            }
        }

        /* mandatory */
        ob_get_s(options, L"jid", &jid);
        ob_get_s(options, L"password", &password);
        ob_get_s(options, L"host", &host);
     
        if(ob_is_defined(options, L"disableTLS"))
        {
            disableTLS = ob_get_b(options, L"disableTLS");
        }
        
        if(ob_is_defined(options, L"mandatoryTLS"))
        {
            mandatoryTLS = ob_get_b(options, L"mandatoryTLS");
        }
        
        if(ob_is_defined(options, L"legacyTLS"))
        {
            legacyTLS = ob_get_b(options, L"legacyTLS");
        }
        
        if(ob_is_defined(options, L"trustTLS"))
        {
            trustTLS = ob_get_b(options, L"trustTLS");
        }

        if(ob_is_defined(options, L"enableLegacyAuth"))
        {
            enableLegacyAuth = ob_get_b(options, L"enableLegacyAuth");
        }
 
    }
    
    PA_CollectionRef log = PA_CreateCollection();
    
    xmpp_log_t log_ctx;
    log_ctx.handler = log_handler;
    log_ctx.userdata = log;
    
    xmpp_ctx_t *ctx = xmpp_ctx_new(NULL, &log_ctx);
    xmpp_conn_t *conn = xmpp_conn_new(ctx);
    
    if (keepAlive) {
        xmpp_conn_set_keepalive(conn, keepalive_timeout, keepalive_interval);
    }
    
    if (disableTLS) {
        xmpp_conn_disable_tls(conn);
        flags = XMPP_CONN_FLAG_DISABLE_TLS;
    }else {
        
        if (mandatoryTLS) {
            flags |= XMPP_CONN_FLAG_MANDATORY_TLS;
        }
        if (legacyTLS) {
            flags |= XMPP_CONN_FLAG_LEGACY_SSL;
        }
        if (trustTLS) {
            flags |= XMPP_CONN_FLAG_TRUST_TLS;
        }
        
    }
    
    if (enableLegacyAuth) {
        flags |= XMPP_CONN_FLAG_LEGACY_AUTH;
    }
    
    xmpp_conn_set_flags(conn, flags);

    xmpp_conn_set_jid(conn, (const char *)jid.c_str());
    xmpp_conn_set_pass(conn, (const char *)password.c_str());
    
    mem.ctx = ctx;
    
    switch (type) {
        case xmpp_stanza_type_message:
            xmpp_connect_client(conn, (const char *)host.c_str(), 0,
                                stanza_type_message_handler, &mem);
            break;
        case xmpp_stanza_type_connect:
            xmpp_connect_client(conn, (const char *)host.c_str(), 0,
                                stanza_type_connect_handler, &mem);
            break;
        default:
            break;
    }
    
    xmpp_ctx_set_timeout(ctx, run_timeout);
    xmpp_run(ctx);
    
    xmpp_conn_release(conn);
    xmpp_ctx_free(ctx);
    
    ob_set_c(status, L"log", log);
    
    switch (type) {
        case xmpp_stanza_type_message:

            break;
        case xmpp_stanza_type_connect:
            ob_set_c(status, L"messages", messages);
            break;
        default:
            break;
    }
    
    PA_ReturnObject(params, status);
}
