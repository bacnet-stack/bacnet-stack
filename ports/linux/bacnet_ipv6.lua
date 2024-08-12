-- bacnet_BIPV6.lua
--
-- Dissector for BACnet/IPv6 [135-2016 Annex X]
--
-- Copyright 2011, Siemens Building Technolgies
-- Maintainer Philippe Goetz <philippe.goetz@siemens.com>
-- Updated by Steve Karg <skarg@users.sourceforge.net>
--
-- SPDX-License-Identifier: GPL-2.0-or-later

-- Add the following to the end of init.lua in the Wireshark install directory:
-- dofile("bacnet_ipv6.lua")


-- [in] tree,pf,buffer,offset,len
-- [out] offset, value, ti, bf
function uint_dissector(tree,pf,buffer,offset,len)
  if (offset + len) <= buffer:len() then
    local bf = buffer(offset,len)
    local ti = nil
    if tree ~= nil then
      ti = tree:add(pf,bf)
    end
    return offset + len, bf:uint(), ti, bf
  end
  local ti = tree:add(pf,buffer(offset))
  ti:add_expert_info(PI_MALFORMED, PI_ERROR, "Data shortage")
  error("Data shortage")
end

function bytes_dissector(tree,pf,buffer,offset,len)
  if (offset + len) <= buffer:len() then
    local bf = buffer(offset,len)
    local ti = nil
    if tree ~= nil then
      ti = tree:add(pf,bf)
    end
    return offset + len, bf:bytes(), ti, bf
  end
  local ti = tree:add(pf,buffer(offset))
  ti:add_expert_info(PI_MALFORMED, PI_ERROR, "Data shortage")
  error("Data shortage")
end

p_BACnetBIPV6 = Proto("lua_BACnetBIPV6", "B/IPv6 BACnet Virtual Link Control");
p_BACnet_Port = Proto("BACnet-Port", "BACnet UDP Port Handoff")
p_dissector_bacnet = Dissector.get("bacnet")
p_dissector_bipv4 = Dissector.get("bvlc")

do
  p_BACnetBIPV6.init = function()
    debug("p_BACnetBIPV6.init")
  end

  local t_BACnetBIPV6_Types = {
    [0x82]="B/IPv6 (Annex X)"
    }
  local t_BACnetBIPV6_Functions = {
	-- Annex X, PPR3 Draft 22
    [0x00]="BVLC-Result",
    [0x01]="Original-Unicast-NPDU",
    [0x02]="Original-Broadcast-NPDU",
    [0x03]="Address-Resolution",
    [0x04]="Forwarded-Address-Resolution",
    [0x05]="Address-Resolution-Ack",
    [0x06]="Virtual-Address-Resolution",
    [0x07]="Virtual-Address-Resolution-Ack",
    [0x08]="Forwarded-NPDU",
    [0x09]="Register-Foreign-Device",
    [0x0A]="Delete-Foreign-Device",
    [0x0B]="Secure-BVLL",
	[0x0C]="Distribute-Broadcast-To-Network"
    }

  local pf_BIPV6Data     = ProtoField.bytes ("BIPV6.data",                                                    "B/IPV6 Data")

  p_BACnetBIPV6.fields = {
    pf_BIPV6Data
    }

  local t_BACnetBIPV6_dissectors = {}

  -- local dt_BIPV6_npdu = DissectorTable.new("lua_BACnetBIPV6.npdu","BIPV6 NPDU",ftypes.UINT8,base.HEX)
  -- BIPV6 header fields
  local pf_BIPV6 = {}
  pf_BIPV6.Type     = ProtoField.uint8 ("BIPV6.type","Type",base.HEX,t_BACnetBIPV6_Types)
  table.insert(p_BACnetBIPV6.fields,pf_BIPV6.Type)
  pf_BIPV6.Function = ProtoField.uint8 ("BIPV6.function","Function",base.HEX,t_BACnetBIPV6_Functions,0,"BIPV6 Function")
  table.insert(p_BACnetBIPV6.fields,pf_BIPV6.Function)
  pf_BIPV6.Length   = ProtoField.uint16("BIPV6.length","BIPV6-Length",base.DEC,nil,0,"Length of BIPV6")
  table.insert(p_BACnetBIPV6.fields,pf_BIPV6.Length)
  pf_BIPV6.ResultCode = ProtoField.uint16("BIPV6.resultCode","Result-Code",base.DEC,nil,0,"Result Code")
  table.insert(p_BACnetBIPV6.fields,pf_BIPV6.ResultCode)
  pf_BIPV6.SourceVirtualAddress = ProtoField.uint24 ("BIPV6.sourceVirtualAddress","Source-Virtual-Address")
  table.insert(p_BACnetBIPV6.fields,pf_BIPV6.SourceVirtualAddress)
  pf_BIPV6.DestinationVirtualAddress = ProtoField.uint24 ("BIPV6.destinationVirtualAddress","Destination-Virtual-Address")
  table.insert(p_BACnetBIPV6.fields,pf_BIPV6.DestinationVirtualAddress)
  pf_BIPV6.OriginalSourceEffectiveAddress = ProtoField.bytes ("BIPV6.originalSourceEffectiveAddress","Original-Source-B/IPv6-Address")
  table.insert(p_BACnetBIPV6.fields,pf_BIPV6.OriginalSourceEffectiveAddress)
  pf_BIPV6.OriginalSourceEffectiveAddress_ip = ProtoField.ipv6 ("BIPV6.originalSourceEffectiveAddress.ip","Original-Source-B/IPv6-Address (ip)")
  table.insert(p_BACnetBIPV6.fields,pf_BIPV6.OriginalSourceEffectiveAddress_ip)
  pf_BIPV6.OriginalSourceEffectiveAddress_port = ProtoField.uint16 ("BIPV6.originalSourceEffectiveAddress.port","Original-Source-B/IPv6-Address (port)")
  table.insert(p_BACnetBIPV6.fields,pf_BIPV6.OriginalSourceEffectiveAddress_port)
  pf_BIPV6.OriginalSourceVirtualAddress = ProtoField.uint24 ("BIPV6.originalSourceVirtualAddress","Original-Source-Virtual-Address")
  table.insert(p_BACnetBIPV6.fields,pf_BIPV6.OriginalSourceVirtualAddress)
  pf_BIPV6.DestinationEffectiveAddress = ProtoField.bytes ("BIPV6.destinationEffectiveAddress","Original-Source-B/IPv6-Address")
  table.insert(p_BACnetBIPV6.fields,pf_BIPV6.DestinationEffectiveAddress)
  pf_BIPV6.DestinationEffectiveAddress_ip = ProtoField.ipv6 ("BIPV6.destinationEffectiveAddress.ip","Original-Source-B/IPv6-Address (ip)")
  table.insert(p_BACnetBIPV6.fields,pf_BIPV6.DestinationEffectiveAddress_ip)
  pf_BIPV6.DestinationEffectiveAddress_port = ProtoField.uint16 ("BIPV6.destinationEffectiveAddress.port","Original-Source-B/IPv6-Address (port)")
  table.insert(p_BACnetBIPV6.fields,pf_BIPV6.DestinationEffectiveAddress_port)
  pf_BIPV6.TimeToLive = ProtoField.uint16 ("BIPV6.timeToLive","Time-To-Live")
  table.insert(p_BACnetBIPV6.fields,pf_BIPV6.TimeToLive)

  function p_BACnetBIPV6.dissector(buffer,pkt,tree)

    pkt.cols["protocol"] = "B/IPv6"
    pkt.cols["info"] = "BACnet Building Automation and Control Network"
    local offset = 0
    local v_BIPV6 = {}
    local ti_BIPV6 = {}
    ti_BIPV6.ti = tree:add(p_BACnetBIPV6,buffer(0))
    offset, v_BIPV6.Type, ti_BIPV6.Type = uint_dissector(ti_BIPV6.ti, pf_BIPV6.Type, buffer, offset, 1)
	--if v_BIPV6.Type == 0x81 then
	--	p_dissector_bipv4:call(buffer, pkt, tree)
	-- elseif v_BIPV6 ~= 0x82 then
	--    ti_BIPV6.Type:add_expert_info(PI_MALFORMED, PI_WARN, "Unknown BVLC Type")
     --   return
	--end
     if v_BIPV6.Type ~= 0x82 then
	    ti_BIPV6.Type:add_expert_info(PI_MALFORMED, PI_WARN, "Unknown BVLC Type")
        return
    end
    offset, v_BIPV6.Function, ti_BIPV6.Function = uint_dissector(ti_BIPV6.ti, pf_BIPV6.Function, buffer, offset, 1)
    offset, v_BIPV6.Length, ti_BIPV6.Length = uint_dissector(ti_BIPV6.ti, pf_BIPV6.Length, buffer, offset, 2)
    local BIPV6Dissector = t_BACnetBIPV6_dissectors[v_BIPV6.Function]
    local npduData = nil
    if BIPV6Dissector ~= nil then
      pkt.cols["info"] = t_BACnetBIPV6_Functions[v_BIPV6.Function]
      offset, npduData = BIPV6Dissector(buffer,pkt,ti_BIPV6.ti,offset,v_BIPV6,tree)
    end
    if npduData ~= nil then
      if p_BACnetNPDU ~= nil then
        ti_BIPV6.ti:set_len(offset)
        p_BACnetNPDU.dissector:call(npduData:tvb(), pkt, tree)
        offset = offset + npduData:len()
      else
        p_dissector_bacnet:call(npduData:tvb(), pkt, tree)
      end
    end
	-- CLB don't show the raw data
    -- if offset ~= buffer:len() then
    --   ti_BIPV6.ti:add(pf_BIPV6Data,buffer(offset))
    -- end
    ti_BIPV6 = nil
    collectgarbage("collect")
  end

  -- [0x00] BVLC-Result
  t_BACnetBIPV6_dissectors[0x00] = function(buffer,pkt,tree,offset,npdu)
    offset = bytes_dissector(tree,pf_BIPV6.SourceVirtualAddress,buffer,offset,3)
    offset = bytes_dissector(tree,pf_BIPV6.ResultCode,buffer,offset,2)
    return offset
  end

  -- [0x01] Original-Unicast-NPDU
  t_BACnetBIPV6_dissectors[0x01] = function(buffer,pkt,tree,offset,npdu)
    offset = bytes_dissector(tree,pf_BIPV6.SourceVirtualAddress,buffer,offset,3)
    offset = bytes_dissector(tree,pf_BIPV6.DestinationVirtualAddress,buffer,offset,3)
    return offset, buffer(offset)
  end

  -- [0x02] Original-Broadcast-NPDU
  t_BACnetBIPV6_dissectors[0x02] = function(buffer,pkt,tree,offset,npdu)
    offset = bytes_dissector(tree,pf_BIPV6.SourceVirtualAddress,buffer,offset,3)
    return offset, buffer(offset)
  end

  -- [0x03] Address-Resolution
  t_BACnetBIPV6_dissectors[0x03] = function(buffer,pkt,tree,offset,npdu)
    offset = bytes_dissector(tree,pf_BIPV6.SourceVirtualAddress,buffer,offset,3)
    offset = bytes_dissector(tree,pf_BIPV6.DestinationVirtualAddress,buffer,offset,3)
    return offset
  end

  -- [0x04] Forwarded-Address-Resolution
  t_BACnetBIPV6_dissectors[0x04] = function(buffer,pkt,tree,offset,npdu)
    offset = bytes_dissector(tree,pf_BIPV6.OriginalSourceVirtualAddress,buffer,offset,3)
    offset = bytes_dissector(tree,pf_BIPV6.DestinationVirtualAddress,buffer,offset,3)
	local subtree = tree:add(pf_BIPV6.OriginalSourceEffectiveAddress,buffer(offset,18))
    offset = bytes_dissector(subtree,pf_BIPV6.OriginalSourceEffectiveAddress_ip,buffer,offset,16)
    offset = uint_dissector(subtree,pf_BIPV6.OriginalSourceEffectiveAddress_port,buffer,offset,2)
    return offset
  end

  -- [0x05] Address-Resolution-Ack
  t_BACnetBIPV6_dissectors[0x05] = function(buffer,pkt,tree,offset,npdu)
    offset = bytes_dissector(tree,pf_BIPV6.SourceVirtualAddress,buffer,offset,3)
    offset = bytes_dissector(tree,pf_BIPV6.DestinationVirtualAddress,buffer,offset,3)
    return offset
  end

  -- [0x06] Virtual-Address-Resolution
  t_BACnetBIPV6_dissectors[0x06] = function(buffer,pkt,tree,offset,npdu)
    offset = bytes_dissector(tree,pf_BIPV6.SourceVirtualAddress,buffer,offset,3)
    return offset
  end

  -- [0x07] Virtual-Address-Resolution-Ack
  t_BACnetBIPV6_dissectors[0x07] = function(buffer,pkt,tree,offset,npdu)
    offset = bytes_dissector(tree,pf_BIPV6.SourceVirtualAddress,buffer,offset,3)
    return offset
  end

  -- [0x08] Forwarded-NPDU
  t_BACnetBIPV6_dissectors[0x08] = function(buffer,pkt,tree,offset,npdu)
    offset = bytes_dissector(tree,pf_BIPV6.OriginalSourceVirtualAddress,buffer,offset,3)
	local subtree = tree:add(pf_BIPV6.OriginalSourceEffectiveAddress,buffer(offset,18))
    offset = bytes_dissector(subtree,pf_BIPV6.OriginalSourceEffectiveAddress_ip,buffer,offset,16)
    offset = uint_dissector(subtree,pf_BIPV6.OriginalSourceEffectiveAddress_port,buffer,offset,2)
    return offset, buffer(offset)
  end

  -- [0x09] Register-Foreign-Device
  t_BACnetBIPV6_dissectors[0x09] = function(buffer,pkt,tree,offset,npdu)
    offset = bytes_dissector(tree,pf_BIPV6.SourceVirtualAddress,buffer,offset,3)
    offset = uint_dissector(tree,pf_BIPV6.TimeToLive,buffer,offset,2)
    return offset
  end

  -- [0x0A] Delete-Foreign-Device
  t_BACnetBIPV6_dissectors[0x0A] = function(buffer,pkt,tree,offset,npdu)
    offset = bytes_dissector(tree,pf_BIPV6.SourceVirtualAddress,buffer,offset,3)
    -- FDT Entry decoding
    return offset
  end

  -- [0x0B] Secure-BVLL
  t_BACnetBIPV6_dissectors[0x0B] = function(buffer,pkt,tree,offset,npdu)
    return offset
  end

  -- [0x0C] Distribute-Broadcast-To-Network
  t_BACnetBIPV6_dissectors[0x0C] = function(buffer,pkt,tree,offset,npdu)
    offset = bytes_dissector(tree,pf_BIPV6.OriginalSourceVirtualAddress,buffer,offset,3)
	local subtree = tree:add(pf_BIPV6.OriginalSourceEffectiveAddress,buffer(offset,18))
    offset = bytes_dissector(subtree,pf_BIPV6.OriginalSourceEffectiveAddress_ip,buffer,offset,16)
    offset = uint_dissector(subtree,pf_BIPV6.OriginalSourceEffectiveAddress_port,buffer,offset,2)
    return offset, buffer(offset)
  end

  function p_BACnet_Port.dissector(tvb, pinfo, tree)
      local BIP_Type = tvb(0,1):uint()
      if BIP_Type == 0x81 then
          p_dissector_bipv4:call(tvb, pinfo, tree)
      elseif BIP_Type == 0x82 then
          p_BACnetBIPV6.dissector(tvb, pinfo, tree)
      end
  end

  -- load the udp.port table
  local dt_udp_port = DissectorTable.get("udp.port")
  -- p_dissector_bvlc = dt_udp_port:get_dissector(47808)
  -- dt_udp_port:remove(47808, p_dissector_bvlc)
  -- dt_udp_port:add(47808, p_BACnetBIPV6)
  dt_udp_port:add(47808, p_BACnet_Port)

end
