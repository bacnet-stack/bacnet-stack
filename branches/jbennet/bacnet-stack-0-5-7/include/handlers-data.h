#ifndef __HANDLERS_DATA_H_
#define __HANDLERS_DATA_H_

/*/ Data for the various bacnet handlers, to prevent */
/*/ multiple global static variables. */
struct bacnet_handlers_data;
struct bacnet_session_object;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    /*/ Initial session create with handlers initialization */
    struct bacnet_session_object *create_bacnet_session(
        );
    /*/ Returns and/or allocates specific bacnet handler data for this particular session */
    struct bacnet_handlers_data *get_bacnet_session_handler_data(
        struct bacnet_session_object *session);
    /*/ Unallocates specific bacnet handler data for this particular session */
    void destroy_bacnet_session_handler_data(
        struct bacnet_session_object *session);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /*__HANDLERS_DATA_H_ */
