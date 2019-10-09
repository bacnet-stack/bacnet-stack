# Unit tests for the BACnet Stack project

LOGFILE = test.log

all: ai ao av bi bo bv csv lc lo lsp \
	mso msv ms-input netport osv piv command \
	access_credential access_door access_point access_rights \
	access_user access_zone credential_data_input

clean: logfile
	rm ${LOGFILE}

logfile:
	touch ${LOGFILE}

report:
	cat ${LOGFILE}

access_credential: logfile access_credential.mak
	$(MAKE) -s -f access_credential.mak clean all
	( ./access_credential >> ${LOGFILE} )
	$(MAKE) -s -f access_credential.mak clean

access_door: logfile access_door.mak
	$(MAKE) -s -f access_door.mak clean all
	( ./access_door >> ${LOGFILE} )
	$(MAKE) -s -f access_door.mak clean

access_point: logfile access_point.mak
	$(MAKE) -s -f access_point.mak clean all
	( ./access_point >> ${LOGFILE} )
	$(MAKE) -s -f access_point.mak clean

access_rights: logfile access_rights.mak
	$(MAKE) -s -f access_rights.mak clean all
	( ./access_rights >> ${LOGFILE} )
	$(MAKE) -s -f access_rights.mak clean

access_user: logfile access_user.mak
	$(MAKE) -s -f access_user.mak clean all
	( ./access_user >> ${LOGFILE} )
	$(MAKE) -s -f access_user.mak clean

access_zone: logfile access_zone.mak
	$(MAKE) -s -f access_zone.mak clean all
	( ./access_zone >> ${LOGFILE} )
	$(MAKE) -s -f access_zone.mak clean

credential_data_input: logfile credential_data_input.mak
	$(MAKE) -s -f credential_data_input.mak clean all
	( ./credential_data_input >> ${LOGFILE} )
	$(MAKE) -s -f credential_data_input.mak clean

ai: logfile ai.mak
	$(MAKE) -s -f ai.mak clean all
	( ./analog_input >> ${LOGFILE} )
	$(MAKE) -s -f ai.mak clean

ao: logfile ao.mak
	$(MAKE) -s -f ao.mak clean all
	( ./analog_output >> ${LOGFILE} )
	$(MAKE) -s -f ao.mak clean

av: logfile av.mak
	$(MAKE) -s -f av.mak clean all
	( ./analog_value >> ${LOGFILE} )
	$(MAKE) -s -f av.mak clean

bi: logfile bi.mak
	$(MAKE) -s -f bi.mak clean all
	$(MAKE) -s -f bi.mak clean

bo: logfile bo.mak
	$(MAKE) -s -f bo.mak clean all
	( ./binary_output >> ${LOGFILE} )
	$(MAKE) -s -f bo.mak clean

bv: logfile bv.mak
	$(MAKE) -s -f bv.mak clean all
	( ./binary_value >> ${LOGFILE} )
	$(MAKE) -s -f bv.mak clean

command: logfile command.mak
	$(MAKE) -s -f command.mak clean all
	( ./command >> ${LOGFILE} )
	$(MAKE) -s -f command.mak clean

csv: logfile csv.mak
	$(MAKE) -s -f csv.mak clean all
	( ./characterstring_value >> ${LOGFILE} )
	$(MAKE) -s -f csv.mak clean

device: logfile device.mak
	$(MAKE) -s -f device.mak clean all
	( ./device >> ${LOGFILE} )
	$(MAKE) -s -f device.mak clean

lc: logfile lc.mak
	$(MAKE) -s -f lc.mak clean all
	( ./load_control >> ${LOGFILE} )
	$(MAKE) -s -f lc.mak clean

lo: logfile lo.mak
	$(MAKE) -s -f lo.mak clean all
	( ./lighting_output >> ${LOGFILE} )
	$(MAKE) -s -f lo.mak clean

lsp: logfile lsp.mak
	$(MAKE) -s -f lsp.mak clean all
	( ./life_safety_point >> ${LOGFILE} )
	$(MAKE) -s -f lsp.mak clean

ms-input: logfile ms-input.mak
	$(MAKE) -s -f ms-input.mak clean all
	( ./multistate_input >> ${LOGFILE} )
	$(MAKE) -s -f ms-input.mak clean

mso: logfile mso.mak
	$(MAKE) -s -f mso.mak clean all
	( ./multistate_output >> ${LOGFILE} )
	$(MAKE) -s -f mso.mak clean

msv: logfile msv.mak
	$(MAKE) -s -f msv.mak clean all
	( ./multistate_value >> ${LOGFILE} )
	$(MAKE) -s -f msv.mak clean

osv: logfile osv.mak
	$(MAKE) -s -f osv.mak clean all
	( ./octetstring_value >> ${LOGFILE} )
	$(MAKE) -s -f osv.mak clean

netport: logfile netport.mak
	$(MAKE) -s -f netport.mak clean all
	( ./network_port >> ${LOGFILE} )
	$(MAKE) -s -f netport.mak clean

piv: logfile piv.mak
	$(MAKE) -s -f piv.mak clean all
	( ./positiveinteger_value >> ${LOGFILE} )
	$(MAKE) -s -f piv.mak clean

schedule: logfile schedule.mak
	$(MAKE) -s -f schedule.mak clean all
	( ./schedule >> ${LOGFILE} )
	$(MAKE) -s -f schedule.mak clean
