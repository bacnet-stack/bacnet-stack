/** Session objects implementation :
 *  1) remove all the global static variables
 *  2) allow multiple instances via multiple session objects
 */
#include <stdlib.h>
#include "session.h"

/* Allocate a new BACnet session */
struct bacnet_session_object *bacnet_allocate_session(
    )
{
    struct bacnet_session_object *session =
        (struct bacnet_session_object *) calloc(1,
        sizeof(struct bacnet_session_object));
    /* Initialize default data values, timeouts, etc.. */
    /* XXX */
    /* APDU */
    session->APDU_Timeout_Milliseconds = 3000;
    session->APDU_Segment_Timeout_Milliseconds = 2000;
    session->APDU_Number_Of_Retries = 3;
    return session;
}

/* Destroy a BACnet session object */
void bacnet_destroy_session(
    struct bacnet_session_object *session_object)
{
    /* Destroy allocated data */
#if (MAX_TSM_TRANSACTIONS)
    int ix;
    for (ix = 0; ix < MAX_TSM_TRANSACTIONS; ix++) {
        free_blob(&session_object->TSM_List[ix]);
    }
#endif
    free(session_object);
}

/* Special synchronization functions */

/* Sleeping ; waiting for an event on this session. NOP if not implemented within callbacks. 
 @param session_object The current session.
 @param millisecond The maximum wait timeout.
 @return true if we were signalled or there were no callbacks, false if we waited all timeout. */
bool bacnet_session_wait(
    struct bacnet_session_object *session_object,
    int milliseconds)
{
#if defined(WITH_SESSION_SYNCHRONISATION)
    if (session_object->session_synchro_wait)
        return session_object->session_synchro_wait(session_object,
            milliseconds);
#endif
    return true;
}

/** Sleeping ; trying to wait for an event on this session, test if we are allowed to do so. 
 @param session_object The current session.
 @return true if we can use 'bacnet_session_wait', false otherwise. */
bool bacnet_session_can_wait(
    struct bacnet_session_object * session_object)
{
#if defined(WITH_SESSION_SYNCHRONISATION)
    if (session_object->session_synchro_can_wait)
        return session_object->session_synchro_can_wait(session_object);
#endif
    return false;
}

/* Sleeping ; signal an event on this session
   @param session_object The current session. */
void bacnet_session_signal(
    struct bacnet_session_object *session_object)
{
#if defined(WITH_SESSION_SYNCHRONISATION)
    if (session_object->session_synchro_signal)
        session_object->session_synchro_signal(session_object);
#endif
}

/* Multi-threading: get a lock on this session object
   @param session_object The current session. */
void bacnet_session_lock(
    struct bacnet_session_object *session_object)
{
#if defined(WITH_SESSION_SYNCHRONISATION)
    if (session_object->session_synchro_lock)
        session_object->session_synchro_lock(session_object);
#endif
}

/* Multi-threading: release a lock on this session object
   @param session_object The current session. */
void bacnet_session_unlock(
    struct bacnet_session_object *session_object)
{
#if defined(WITH_SESSION_SYNCHRONISATION)
    if (session_object->session_synchro_unlock)
        session_object->session_synchro_unlock(session_object);
#endif
}

/* Log: outputs a log message from the core of the bacnet-stack
   @param message The log message.
   @param level The log level.
   @param session_object The current session. */
void bacnet_session_log(
    struct bacnet_session_object *session_object,
    int level,
    const char *message,
    BACNET_ADDRESS * addressinfo,
    int invokeIdInfo)
{
#if defined(WITH_SESSION_LOG)
    if (session_object->session_log)
        session_object->session_log(session_object, level, message,
            addressinfo, invokeIdInfo);
#endif
}
