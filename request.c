//
// request.c: Does the bulk of the work for the web server.
// 

#include "segel.h"
#include "request.h"

// requestError(      fd,    filename,        "404",    "Not found", "OS-HW3 Server could not find this file");
void requestError(int fd, char *reason, char *errorNum, char *shortMsg, char *longMsg,Stats stats)
{
   char buffer[MAXLINE], str[MAXBUF];

   // Create the body of the error message
   sprintf(str, "<html><title>OS-HW3 Error</title>");
   sprintf(str, "%s<body bgcolor=""fffff"">\r\n", str);
   sprintf(str, "%s%s: %s\r\n", str, errorNum, shortMsg);
   sprintf(str, "%s<p>%s: %s\r\n", str, longMsg, reason);
   sprintf(str, "%s<hr>OS-HW3 Web Server\r\n", str);

   // Write out the header information for this response
   sprintf(buffer, "HTTP/1.0 %s %s\r\n", errorNum, shortMsg);
   Rio_writen(fd, buffer, strlen(buffer));
   printf("%s", buffer);

   sprintf(buffer, "Content-Type: text/html\r\n");
   Rio_writen(fd, buffer, strlen(buffer));
   printf("%s", buffer);

   sprintf(buffer, "Content-Length: %lu\r\n", strlen(str));

   sprintf(buffer, "%sStat-Req-Arrival:: %lu.%06lu\r\n", buffer,stats->arrival.tv_sec,stats->arrival.tv_usec);
   sprintf(buffer, "%sStat-Req-Dispatch:: %lu.%06lu\r\n", buffer,stats->dispatch.tv_sec,stats->dispatch.tv_usec);
   sprintf(buffer, "%sStat-Thread-Id:: %d\r\n", buffer, stats->handlerId);
   sprintf(buffer, "%sStat-Thread-Count:: %d\r\n", buffer,stats->handlerReqCount);
   sprintf(buffer, "%sStat-Thread-Static:: %d\r\n", buffer,stats->handlerStaticReqCount);
   sprintf(buffer, "%sStat-Thread-Dynamic:: %d\r\n\r\n", buffer,stats->handlerDynamicReqCount);

   Rio_writen(fd, buffer, strlen(buffer));

   Rio_writen(fd, str, strlen(str));
   printf("%s", str);

}


void readReq(rio_t *rp)
{
   char buffer[MAXLINE];

   Rio_readlineb(rp, buffer, MAXLINE);
   while (strcmp(buffer, "\r\n")) {
      Rio_readlineb(rp, buffer, MAXLINE);
   }
   return;
}


int parseURI(char *uri, char *fileName, char *args)
{
   char *p;

   if (strstr(uri, "..")) {
      sprintf(fileName, "./public/home.html");
      return 1;
   }

   if (!strstr(uri, "cgi")) {
      // static
      strcpy(args, "");
      sprintf(fileName, "./public/%s", uri);
      if (uri[strlen(uri)-1] == '/') {
         strcat(fileName, "home.html");
      }
      return 1;
   } else {
      // dynamic
      p = index(uri, '?');
      if (p) {
         strcpy(args, p+1);
         *p = '\0';
      } else {
         strcpy(args, "");
      }
      sprintf(fileName, "./public/%s", uri);
      return 0;
   }
}

//
// Fills in the filetype given the filename
//
void getFiletype(char *fileName, char *fileType)
{
    if (strstr(fileName, ".jpg"))
      strcpy(fileType, "image/jpeg");
    else if (strstr(fileName, ".gif"))
        strcpy(fileType, "image/gif");
    else if (strstr(fileName, ".html"))
        strcpy(fileType, "text/html");
    else
      strcpy(fileType, "text/plain");
}

void dynamicServe(int fd, char *fileName, char *args,Stats stats)
{
   char buffer[MAXLINE], *list[] = {NULL};

   sprintf(buffer, "HTTP/1.0 200 OK\r\n");
   sprintf(buffer, "%sServer: OS-HW3 Web Server\r\n", buffer);

   sprintf(buffer, "%sStat-Req-Arrival:: %lu.%06lu\r\n", buffer,stats->arrival.tv_sec,stats->arrival.tv_usec);
   sprintf(buffer, "%sStat-Req-Dispatch:: %lu.%06lu\r\n", buffer,stats->dispatch.tv_sec,stats->dispatch.tv_usec);
   sprintf(buffer, "%sStat-Thread-Id:: %d\r\n", buffer, stats->handlerId);
   sprintf(buffer, "%sStat-Thread-Count:: %d\r\n", buffer,stats->handlerReqCount);
   sprintf(buffer, "%sStat-Thread-Static:: %d\r\n", buffer,stats->handlerStaticReqCount);
   sprintf(buffer, "%sStat-Thread-Dynamic:: %d\r\n", buffer,stats->handlerDynamicReqCount);
   
   Rio_writen(fd, buffer, strlen(buffer));

   pid_t pid;
   if ((pid = Fork()) == 0) {
      /* Child process */
      Setenv("QUERY_STRING", args, 1);
      /* When the CGI process writes to stdout, it will instead go to the socket */
      Dup2(fd, STDOUT_FILENO);
      Execve(fileName, list, environ);
   }
   waitpid(pid, NULL, 0);
}


void requestServeStatic(int fd, char *fileName, int fileSize,Stats stats)
{
    //printf("static\n");
   int sourceFD;
   char *sourceP, typeOfFile[MAXLINE], buf[MAXBUF];

   getFiletype(fileName, typeOfFile);

   sourceFD = Open(fileName, O_RDONLY, 0);

   sourceP = Mmap(0, fileSize, PROT_READ, MAP_PRIVATE, sourceFD, 0);
   Close(sourceFD);

   // put together response
   sprintf(buf, "HTTP/1.0 200 OK\r\n");
   sprintf(buf, "%sServer: OS-HW3 Web Server\r\n", buf);
   sprintf(buf, "%sContent-Length: %d\r\n", buf, fileSize);
   sprintf(buf, "%sContent-Type: %s\r\n", buf, typeOfFile);
    
   sprintf(buf, "%sStat-Req-Arrival:: %lu.%06lu\r\n", buf,stats->arrival.tv_sec,stats->arrival.tv_usec);
   sprintf(buf, "%sStat-Req-Dispatch:: %lu.%06lu\r\n", buf,stats->dispatch.tv_sec,stats->dispatch.tv_usec);
   sprintf(buf, "%sStat-Thread-Id:: %d\r\n", buf, stats->handlerId);
   sprintf(buf, "%sStat-Thread-Count:: %d\r\n", buf,stats->handlerReqCount);
   sprintf(buf, "%sStat-Thread-Static:: %d\r\n", buf,stats->handlerStaticReqCount);
   sprintf(buf, "%sStat-Thread-Dynamic:: %d\r\n\r\n", buf,stats->handlerDynamicReqCount);

   Rio_writen(fd, buf, strlen(buf));

   Rio_writen(fd, sourceP, fileSize);
   Munmap(sourceP, fileSize);

}

// handle the request
void requestHandle(int fd,Stats stats)
{
   stats->handlerReqCount++;
   int isStatic;
   struct stat statBuffer;
   char buffer[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
   char fileName[MAXLINE], args[MAXLINE];
   rio_t rio;

   Rio_readinitb(&rio, fd);
   Rio_readlineb(&rio, buffer, MAXLINE);
   sscanf(buffer, "%s %s %s", method, uri, version);

   printf("%s %s %s\n", method, uri, version);

   if (strcasecmp(method, "GET")) {
      //stats->handler_thread_static_req_count--;
      requestError(fd, method, "501", "Not Implemented", "OS-HW3 Server does not implement this method",stats);
      return;
   }
   readReq(&rio);

   isStatic = parseURI(uri, fileName, args);
   if (stat(fileName, &statBuffer) < 0) {
      requestError(fd, fileName, "404", "Not found", "OS-HW3 Server could not find this file",stats);
      return;
   }

   if (isStatic) {
      if (!(S_ISREG(statBuffer.st_mode)) || !(S_IRUSR & statBuffer.st_mode)) {
         requestError(fd, fileName, "403", "Forbidden", "OS-HW3 Server could not read this file",stats);
         return;
      }
      stats->handlerStaticReqCount++;
      requestServeStatic(fd, fileName, statBuffer.st_size,stats);
   } else {
      if (!(S_ISREG(statBuffer.st_mode)) || !(S_IXUSR & statBuffer.st_mode)) {
         requestError(fd, fileName, "403", "Forbidden", "OS-HW3 Server could not run this CGI program",stats);
         return;
      }
      stats->handlerDynamicReqCount++;
      dynamicServe(fd, fileName, args,stats);
   }
}


