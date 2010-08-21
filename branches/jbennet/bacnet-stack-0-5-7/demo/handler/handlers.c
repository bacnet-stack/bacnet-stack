#include "handlers-data-core.h"
#include "handlers-data.h"
#include "bacnet-session.h"
#include "session.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*/ Initial session create with handlers initialization */
    struct bacnet_session_object *create_bacnet_session(
        ) {
        /* allocate and initialize session data */
        struct bacnet_session_object *sess = bacnet_allocate_session();
        /* Initialize handlers for this session */
                              get_bacnet_session_handler_data(
            sess);
                              return sess;
    }
/*/ Returns and/or allocates specific bacnet handler data for this particular session */
        struct bacnet_handlers_data *get_bacnet_session_handler_data(
        struct bacnet_session_object *session) {
        /* Déjà alloué ou non ? */
        if (!session->Handler_data)
            session->Handler_data =
                calloc(1, sizeof(struct bacnet_handlers_data));
        return (struct bacnet_handlers_data *) session->Handler_data;
    }

/*/ Unallocates specific bacnet handler data for this particular session */
    void destroy_bacnet_session_handler_data(
        struct bacnet_session_object *session) {
        struct bacnet_handlers_data *h =
            (struct bacnet_handlers_data *) session->Handler_data;
        if (h)
            free(h);
        session->Handler_data = NULL;
    }

#ifdef __cplusplus
}
#endif /* __cplusplus */
