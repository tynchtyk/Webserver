#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <string.h>
#include <string>
#include "threadpool/ThreadPool.h"
#define PORT 5000

void *respond (int sock);
char *ROOT;

int main( int argc, char *argv[] ) {
      int sockfd, newsockfd, portno = PORT;
      socklen_t clilen;
      struct sockaddr_in serv_addr, cli_addr;
      clilen = sizeof(cli_addr);
      ROOT = getenv("PWD");

      /* First call to socket() function */
      sockfd = socket(AF_INET, SOCK_STREAM, 0);

      if (sockfd < 0) {
            perror("ERROR opening socket");
            exit(1);
      }

      // port reusable
      int tr = 1;
      if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &tr, sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
      }

      /* Initialize socket structure */
      bzero((char *) &serv_addr, sizeof(serv_addr));

      serv_addr.sin_family = AF_INET;
      serv_addr.sin_addr.s_addr = INADDR_ANY;
      serv_addr.sin_port = htons(portno);

      /* TODO : Now bind the host address using bind() call.*/
      if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
              perror("ERROR on binding");
              exit(1);
      }


      /* TODO : listen on socket you created */
      listen(sockfd,5);


      printf("Server is running on port %d\n", portno);

      ThreadPool th(2);

      while (1) {
            clilen = sizeof(cli_addr);
            newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
            if (newsockfd < 0) {
                  perror("ERROR on accept");
                  exit(1);
            }
            th.enqueue([newsockfd] { respond(newsockfd ); });
      }

      return 0;
}
void *respond(int sock) {
      int n;
      char buffer[9999];
      bzero(buffer,9999);
      n = recv(sock,buffer,9999, 0);
      
      if (n < 0) {
            printf("recv() error\n");
            return NULL;
      } 
      else if (n == 0) {
            printf("Client disconnected unexpectedly\n");
            return NULL;
      } 

      else { 
            //PARSING THE REQUEST
            char* method = strtok(buffer, " \n\t");
            std::string PATH;
            int index = 0;
            
            while (method != NULL) {
                 if(index == 1) {
                      std::string parsed_i (method);
                      PATH = parsed_i;
                 }
                 index++;
                 method = strtok(NULL, " \n\t"); 
            }

            // GET PATH
            std::string abs_path(ROOT);
            std::string default_file = "/index.html";
            (PATH.size() == 1) ? abs_path += default_file : abs_path += PATH; 


            
            //GET FILE CONTENT AND CONSTRUCT HEADER MESSAGE
            std::string CONTENT = "", Content_Type, Content_Encoding, HEADER = "HTTP/1.1 200 OK \r\n";   
            char * sf = new char[999999];
            int LEN;            
            
            FILE *File = fopen(abs_path.c_str(), "rb");
            
            if (!File) {
                printf("ERROR ON OPEING FILE\n");
                fclose(File);
                return NULL;
            }
                
            if (abs_path.find(".jpg") == std::string::npos) {
                  int c;
                  while ((c = getc(File)) > 0) 
                     CONTENT += char (c);
                  Content_Type = "text/html";                  
                  Content_Encoding = "UTF-8";
                  LEN = strlen(CONTENT.c_str());
            }
            else {
                  fseek(File, 0, SEEK_END);
                  int len = ftell(File);
                  fseek(File, 0, SEEK_SET);
                  Content_Type = "image/jpeg";                  
                  Content_Encoding = "binary";                  
                  LEN = fread(sf, 1, len, File);
            }

            HEADER += "Content-Type: " + Content_Type + " \r\n";
            HEADER += "Content-Encoding: " + Content_Encoding + " \r\n";
            HEADER += "Connection: close \r\n";
            HEADER += "\r\n";
            
            n = send(sock, HEADER.c_str(), strlen(HEADER.c_str()), 0);
            (Content_Type == "text/html") ? n = send(sock, CONTENT.c_str(), LEN, 0) : n = send(sock, sf, LEN, 0);
            
            fclose(File);
            delete  sf;

      }

      shutdown(sock, SHUT_RDWR);
      close(sock);
      return NULL;
}