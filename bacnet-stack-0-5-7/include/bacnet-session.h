#ifndef __BACNET_DEF_SESSION_H_
#define __BACNET_DEF_SESSION_H_

/** BACnet session defines. */

/*/ BACnet session structure */
struct bacnet_session_object;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    /*/ Allocate a new BACnet session */
    struct bacnet_session_object *bacnet_allocate_session(
        );
    /*/ Destroy a BACnet session object */
    void bacnet_destroy_session(
        struct bacnet_session_object *session_object);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /*__BACNET_DEF_SESSION_H_ */
