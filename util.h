#ifndef util_h_INCLUDED
#define util_h_INCLUDED

#define handle_error(msg) \
           do { perror(msg); exit(EXIT_FAILURE); } while (0)

#endif // util_h_INCLUDED

