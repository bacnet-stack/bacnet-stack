#ifndef CLIENTSUBSCRIBEINVOKER_H
#define CLIENTSUBSCRIBEINVOKER_H

/*/ Registers a client subscriber */
struct ClientSubscribeInvoker {
    /*/ Contextual pointer data */
    void *context;
    /*/ Register an invoker function */
    void (
        *SubscribeInvokeId) (
        int invokeId,
        void *context);
};

#endif /*CLIENTSUBSCRIBEINVOKER_H */
