
MutexLocker::MutexLocker(pthread_mutex_t *mutex)
{
    mutex_ = mutex;
    pthread_mutex_lock (mutex_);
}

MutexLocker::~MutexLocker()
{
    phtread_mutex_unlock (mutex_);
}

gssize
pipe_write_n(int fd, const void *vptr, size_t n)
{
    size_t      nleft;
    gssize     nwritten;
    const char  *ptr;

    ptr = vptr;
    nleft = n;
    while (nleft > 0) {
        if ( (nwritten = write(fd, ptr, nleft)) <= 0)
        {
            if (nwritten < 0 && errno == EINTR)
                nwritten = 0;       /* and call write() again */
            else
                return(-1);         /* error */
        }

        nleft -= nwritten;
        ptr   += nwritten;
    }
    return(n);
}

gssize
pipe_read_n(int fd, void *vptr, size_t n)
{
    size_t  nleft;
    gssize nread;
    char    *ptr;

    ptr = vptr;
    nleft = n;
    while (nleft > 0) {
        if ( (nread = read(fd, ptr, nleft)) < 0) {
            if (errno == EINTR)
                nread = 0;      /* and call read() again */
            else
                return(-1);
        } else if (nread == 0)
            break;              /* EOF */

        nleft -= nread;
        ptr   += nread;
    }
    return(n - nleft);      /* return >= 0 */
}
