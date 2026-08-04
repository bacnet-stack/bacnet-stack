# @file
# @brief Build-time BACnet source wiring for PlatformIO environments
# @author Kato Gangstad

Import("env")

from pathlib import Path

PIOENV = env.get("PIOENV", "")
if PIOENV not in (
    "m5stamplc-mstp",
    "m5stamplc-bip",
    "esp32-poe-wifi-bip",
    "esp32-poe-eth-bip",
    "xiao-esp32c3-wifi-bip",
    "m5stamplc-gateway-bip-mstp",
):
    Return()

project_dir = Path(env.subst("$PROJECT_DIR")).resolve()
bacnet_src_root = (project_dir / ".." / ".." / "src").resolve()

bacnet_common_sources = [
    "bacnet/basic/binding/address.c",
    "bacnet/basic/server/bacnet_device.c",
    "bacnet/basic/object/ai.c",
    "bacnet/basic/object/bi.c",
    "bacnet/basic/object/bo.c",
    "bacnet/basic/npdu/h_npdu.c",
    "bacnet/basic/service/h_apdu.c",
    "bacnet/basic/service/h_dcc.c",
    "bacnet/basic/service/h_noserv.c",
    "bacnet/basic/service/h_rd.c",
    "bacnet/basic/service/h_rp.c",
    "bacnet/basic/service/h_rpm.c",
    "bacnet/basic/service/h_whohas.c",
    "bacnet/basic/service/h_whois.c",
    "bacnet/basic/service/h_wp.c",
    "bacnet/basic/service/s_iam.c",
    "bacnet/basic/service/s_ihave.c",
    "bacnet/basic/sys/datetime_mstimer.c",
    "bacnet/basic/sys/days.c",
    "bacnet/basic/sys/dst.c",
    "bacnet/basic/sys/debug.c",
    "bacnet/basic/sys/ringbuf.c",
    "bacnet/basic/sys/fifo.c",
    "bacnet/basic/sys/mstimer.c",
    "bacnet/basic/tsm/tsm.c",
    "bacnet/abort.c",
    "bacnet/bacaction.c",
    "bacnet/bacaddr.c",
    "bacnet/bacapp.c",
    "bacnet/bacdcode.c",
    "bacnet/bacdest.c",
    "bacnet/bacdevobjpropref.c",
    "bacnet/bacerror.c",
    "bacnet/bacint.c",
    "bacnet/bacreal.c",
    "bacnet/bacstr.c",
    "bacnet/datetime.c",
    "bacnet/dcc.c",
    "bacnet/hostnport.c",
    "bacnet/iam.c",
    "bacnet/ihave.c",
    "bacnet/indtext.c",
    "bacnet/memcopy.c",
    "bacnet/npdu.c",
    "bacnet/proplist.c",
    "bacnet/rd.c",
    "bacnet/reject.c",
    "bacnet/rp.c",
    "bacnet/rpm.c",
    "bacnet/timestamp.c",
    "bacnet/whohas.c",
    "bacnet/whois.c",
    "bacnet/wp.c",
    "bacnet/cov.c",
    "bacnet/basic/sys/keylist.c",
]

bacnet_transport_sources = {
    "m5stamplc-mstp": [
        "bacnet/datalink/cobs.c",
        "bacnet/datalink/crc.c",
        "bacnet/datalink/dlmstp.c",
        "bacnet/datalink/mstp.c",
        "bacnet/datalink/mstptext.c",
    ],
    "m5stamplc-bip": [
        "bacnet/datalink/bvlc.c",
    ],
    "esp32-poe-wifi-bip": [
        "bacnet/datalink/bvlc.c",
    ],
    "esp32-poe-eth-bip": [
        "bacnet/datalink/bvlc.c",
    ],
    "xiao-esp32c3-wifi-bip": [
        "bacnet/datalink/bvlc.c",
    ],
    "m5stamplc-gateway-bip-mstp": [
        "bacnet/datalink/bvlc.c",
        "bacnet/datalink/cobs.c",
        "bacnet/datalink/crc.c",
        "bacnet/datalink/dlmstp.c",
        "bacnet/datalink/mstp.c",
        "bacnet/datalink/mstptext.c",
    ],
}

bacnet_sources = bacnet_common_sources + bacnet_transport_sources.get(PIOENV, [])

env.Append(CPPPATH=[str(project_dir / "src"), str(bacnet_src_root)])

for rel in bacnet_sources:
    env.BuildSources(
        str(Path("$BUILD_DIR") / "bacnet"),
        str(bacnet_src_root),
        src_filter=f"+<{rel}>",
    )
