#include "lso.h"
#include "bacdcode.h"
#include "apdu.h"

int lso_encode_adpu(
    uint8_t * apdu,
    uint8_t invoke_id,
    BACNET_LSO_DATA * data)
{
    int len = 0;        /* length of each encoding */
    int apdu_len = 0;   /* total length of the apdu, return value */
    uint16_t max_apdu = Device_Max_APDU_Length_Accepted();


    if (apdu && data) {
        apdu[0] = PDU_TYPE_CONFIRMED_SERVICE_REQUEST;
        apdu[1] = encode_max_segs_max_apdu(0, max_apdu);
        apdu[2] = invoke_id;
        apdu[3] = SERVICE_CONFIRMED_LIFE_SAFETY_OPERATION;
        apdu_len = 4;
        /* tag 0 - requestingProcessId */
        len =
            encode_context_unsigned(&apdu[apdu_len], 0,
            data->processId);
        apdu_len += len;
        /* tag 1 - requestingSource */
        len =
            encode_context_character_string(&apdu[apdu_len], 1,
            &data->requestingSrc);
        apdu_len += len;
        /*
		Operation			
         */
		len = encode_context_enumerated(&apdu[apdu_len], 2, data->operation);
        apdu_len += len;
        /*
		Object ID
         */

		len = encode_context_object_id(&apdu[apdu_len], 3, 
			data->targetObject.type,
			data->targetObject.instance);

        apdu_len += len;
    }

    return apdu_len;
}

int lso_decode_service_request(
        uint8_t                      *apdu,
        unsigned                     apdu_len,
		BACNET_LSO_DATA              *data)
{
    int len = 0;        /* return value */
	int section_length;
    uint8_t tag_number = 0;
    uint32_t len_value_type = 0;
    int type = 0;       /* for decoding */
    int property = 0;   /* for decoding */

    /* check for value pointers */
    if (apdu_len && data) {
        /* Tag 0: Object ID          */

		if ( (section_length = decode_context_unsigned(&apdu[len], 0, &data->processId)) == -1)
		{
			return -1;
		}
		len += section_length;

		if ( (section_length = decode_context_character_string(&apdu[len], 1, &data->requestingSrc)) == -1)
		{
			return -1;
		}
		len += section_length;

		if ( (section_length = decode_context_enumerated(&apdu[len], 2, (int*)&data->operation)) == -1)
		{
			return -1;
		}
		len += section_length;

		/*
		** This is an optional parameter, so dont fail if it doesnt exist
		*/
		if ( decode_is_context_tag(&apdu[len], 3) )
		{
			if ( (section_length = decode_context_object_id(&apdu[len], 3, 
					&data->targetObject.type,
					&data->targetObject.instance)) == -1 )
			{
				return -1;
			}
			len += section_length;
		}
		else
		{
			data->targetObject.type = 0;
			data->targetObject.instance = 0;
		}
		return len;

	}
	
	return 0;
}

#ifdef TEST
#include <assert.h>
#include <string.h>
#include "ctest.h"
#include "bacapp.h"

void testLSO(
    Test * pTest)
{
	uint8_t apdu[1000];
	int len;

	BACNET_LSO_DATA data;
	BACNET_LSO_DATA rxdata;

	memset(&rxdata, 0, sizeof(rxdata));


	characterstring_init_ansi(&data.requestingSrc, "foobar");
	data.operation = LIFE_SAFETY_OP_RESET;
	data.processId = 0x1234;
	data.targetObject.instance = 0x1000;
	data.targetObject.type = OBJECT_BINARY_INPUT;

	len = lso_encode_adpu(apdu, 100, &data);

	lso_decode_service_request(&apdu[4], len, &rxdata);

    ct_test(pTest, data.operation == rxdata.operation);
    ct_test(pTest, data.processId == rxdata.processId);
    ct_test(pTest, data.targetObject.instance == rxdata.targetObject.instance);
    ct_test(pTest, data.targetObject.type == rxdata.targetObject.type);
	ct_test(pTest, memcmp(data.requestingSrc.value, rxdata.requestingSrc.value, rxdata.requestingSrc.length) == 0);
}

#ifdef TEST_LSO
int main(
    int argc,
    char *argv[])
{
    Test *pTest;
    bool rc;

    pTest = ct_create("BACnet Life Safety Operation", NULL);
    /* individual tests */
    rc = ct_addTestFunction(pTest, testLSO);
    assert(rc);

    ct_setStream(pTest, stdout);
    ct_run(pTest);
    (void) ct_report(pTest);
    ct_destroy(pTest);

    return 0;
}
#endif /* TEST_COV */
#endif /* TEST */
