static inline void MaskTerminationSignalsInThread()
{
    /* Mask termination signals in a thread so that they always end up in the
     * main thread. The function calls below should just always succeed and even
     * if they didn't, there's nothing to do. */
    sigset_t th_sigset;
    NDEBUG_UNUSED int ret = sigemptyset(&th_sigset);
    assert(ret == 0);
    ret = sigaddset(&th_sigset, SIGINT);
    assert(ret == 0);
    ret = sigaddset(&th_sigset, SIGTERM);
    assert(ret == 0);
    ret = pthread_sigmask(SIG_BLOCK, &th_sigset, NULL);
    assert(ret == 0);
}
