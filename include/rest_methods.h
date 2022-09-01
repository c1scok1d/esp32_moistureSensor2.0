#ifndef _REST_METHODS_H
#define _REST_METHODS_H

#include <WString.h>

#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus

    int POST(String server_uri, String to_send);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // _REST_METHODS_H