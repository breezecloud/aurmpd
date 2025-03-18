extern "C" {
    #include <stdlib.h>
    #include "mongoose.h"
    #include "mpd_client.h"   
}

#include "http_server.hpp"
static const char *s_listen_on = "ws://0.0.0.0:8600";
static int s_debug_level = MG_LL_DEBUG; //MG_LL_NONE, MG_LL_ERROR, MG_LL_INFO, MG_LL_DEBUG, MG_LL_VERBOSE
static const char *s_root_dir = "./htdocs";

int force_exit = 0;

void bye(){
    force_exit = 1;
}

static void start_thread(void *(*f)(void *), void *p) {
    #ifdef _WIN32
      _beginthread((void(__cdecl *)(void *)) f, 0, p);
    #else
    #define closesocket(x) close(x)
    #include <pthread.h>
      pthread_t thread_id = (pthread_t) 0;
      pthread_attr_t attr;
      (void) pthread_attr_init(&attr);
      (void) pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
      pthread_create(&thread_id, &attr, f, p);
      pthread_attr_destroy(&attr);
    #endif
    }
    
    static void *thread_function(void *param) {
      struct thread_data *p = (struct thread_data *) param;
      MG_INFO(("thread started: "));
      for (;;) {
        sleep(1);
        mpd_poll(p);
        //mg_wakeup(p->mgr, p->conn_id, "hi!", 3);  // Send to parent
      }
      // Free all resources that were passed to us
      free(p);
      return NULL;
    }

static void server_callback(struct mg_connection *c, int ev, void *ev_data) {
    struct mg_http_message *hm;
    struct mg_ws_message *wm = (struct mg_ws_message *) ev_data;
    switch(ev) {
        case MG_EV_OPEN:
            if(c->is_listening){
                // Start worker thread
                struct thread_data *data = ( thread_data *)calloc(1, sizeof(*data));  // Worker owns it
                data->conn_id = c->id;
                data->mgr = c->mgr;
                c->fn_data = data;
                start_thread(thread_function, data);  // Start thread and pass data        
            }
            break;
        case MG_EV_HTTP_MSG:
            hm = (struct mg_http_message *) ev_data;
            if (mg_match(hm->uri, mg_str("/ws"), NULL)){ 
            // Upgrade to websocket. From now on, a connection is a full-duplex
            // Websocket connection, which will receive MG_EV_WS_MSG events.
                mg_ws_upgrade(c, hm, NULL);
                c->data[0] = 'W';    // Set some unique mark on a connection
            }else{
                callback_http(c,hm,s_root_dir);
            }
            MG_INFO(("%.*s %.*s %lu -> %.*s %lu", hm->method.len, hm->method.buf,
                    hm->uri.len, hm->uri.buf, hm->body.len, 3, c->send.buf + 9,
                    c->send.len));  
            break;
        case MG_EV_CLOSE:
            break;
        case MG_EV_WS_MSG:
            MG_INFO(("mg_ws_message : %s len:%d", wm->data.buf,wm->data.len));
            callback_mpd(c,wm);
            //mg_ws_send(c, wm->data.buf, wm->data.len, WEBSOCKET_OP_TEXT); 返回客户端，做测试时使用
            break;
        case MG_EV_WAKEUP:
            struct mg_str *data = (struct mg_str *) ev_data;
            // Broadcast message to all connected websocket clients.
            // Traverse over all connections
            for (struct mg_connection *wc = c->mgr->conns; wc != NULL; wc = wc->next) {
              // Send only to marked connections
              if (wc->data[0] == 'W')
                mg_ws_send(wc, data->buf, data->len, WEBSOCKET_OP_TEXT);
            }
            break;      
    }
}

int main(int argc, char **argv)
{
    struct mg_mgr mgr;  // Mongoose 事件管理器
    struct mg_connection *c;  

    mg_log_set(s_debug_level);  

    mpd.port = 6600;
    strcpy(mpd.host, "127.0.0.1");
    
    mg_mgr_init(&mgr);  // 初始化事件管理器
    // 设置 HTTP 监听器
    
    if ((c = mg_http_listen(&mgr, s_listen_on, server_callback, &mgr)) == NULL) {
        MG_ERROR(("Cannot listen on %s. Use http://ADDR:PORT or :PORT",
                s_listen_on));
        exit(EXIT_FAILURE);     
    }

    MG_INFO(("Mongoose version : v%s", MG_VERSION));
    MG_INFO(("Listening on     : %s", s_listen_on));
    MG_INFO(("Web root         : [%s]", s_root_dir));    
    mg_wakeup_init(&mgr);  // Initialise wakeup socket pair
    // 处理事件，超时时间为 200 毫秒
    while (!force_exit) {
        mg_mgr_poll(&mgr, 200);
    }    
    // 清理资源
    mpd_disconnect();
    mg_mgr_free(&mgr);
    return EXIT_SUCCESS;
}
