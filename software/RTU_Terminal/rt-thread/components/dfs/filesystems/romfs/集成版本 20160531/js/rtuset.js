
var xTcpipCfgList = new Array();
var xUartCfgList = new Array();
var xVarManageExtDataBase = new Array();
var xProtoDevList = new Array();

function createXMLHttpRequest() {
    var request = false;
    if(window.ActiveXObject) {
        var versions = ['Microsoft.XMLHTTP', 'MSXML.XMLHTTP', 'Microsoft.XMLHTTP', 'Msxml2.XMLHTTP.7.0', 'Msxml2.XMLHTTP.6.0', 'Msxml2.XMLHTTP.5.0', 'Msxml2.XMLHTTP.4.0', 'MSXML2.XMLHTTP.3.0', 'MSXML2.XMLHTTP'];
        for(var i=0; i<versions.length; i++) {
            try {
                request = new ActiveXObject(versions[i]);
                if(request) {
                    return request;
                }
            } catch(e) {}
        }
    } else if(window.XMLHttpRequest) {
        request = new XMLHttpRequest();
    }
    return request;
}

function getNumber( id )
{
    return Number(window.document.getElementById(id).value);
}

function getValue( id )
{
    return window.document.getElementById(id).value;
}


function setValue( id, val )
{
    window.document.getElementById(id).value=val;
}

function setEnable( id, enable )
{
    window.document.getElementById(id).disabled=!enable;
}

function setVisible( id, visible )
{
    window.document.getElementById(id).style.visibility=visible?"visible":"hidden";
}

function MyGetJSON(msg,url,data,callback)
{
    var flag = (msg != "" && msg != null);
    if(flag) Show(msg);
    $.ajax({ 
    url: url, 
    async: true,
    dataType: 'json', 
    data: data, 
    success: function(data){
        if(flag) Close();
        if(callback!=null) callback(data);
    },
    timeout: 5000,
    error: function( ){
        if(flag) Close();
    }
    });
}

function initSetting()
{
    getDevInfo();
    for(var i = 0; i < 4; i++) {
        onUartCfgMsChange(i);
    }
    for(var i = 0; i < 8; i++) {
        onAiTypeChange(i);
    }
}

function onUartCfgMsChange(n)
{
    var po = getNumber('uart' + n + '_cfg_proto');
    var ms = getNumber('uart' + n + '_cfg_ms');
    
    if( ms != null && po != null && po == 0 && ms == 0 ) {
        setVisible( 'uart'+ n + '_cfg_addr_lb', true );
        setVisible( 'uart'+ n + '_cfg_addr', true );
    } else {
        setVisible( 'uart'+ n + '_cfg_addr_lb', false );
        setVisible( 'uart'+ n + '_cfg_addr', false );
    }
}

function setUartHtml(n, bd, ut, po, py, ms, ad)
{
    if( bd != null ) setValue('uart' + n + '_cfg_baud', bd); 
    if( ut != null ) setValue('uart' + n + '_cfg_mode', ut); 
    if( po != null ) setValue('uart' + n + '_cfg_proto', po); 
    if( py != null ) setValue('uart' + n + '_cfg_parity', py); 
    if( ms != null ) setValue('uart' + n + '_cfg_ms', ms); 
    if( ad != null ) setValue('uart' + n + '_cfg_addr', ad);
    
    onUartCfgMsChange(n);
}

function setUartCfg(i)
{
    var setval = {
        n:Number(i), 
        bd:getNumber('uart' + i + '_cfg_baud'), 
        ut:getNumber('uart' + i + '_cfg_mode'), 
        po:getNumber('uart' + i + '_cfg_proto'), 
        py:getNumber('uart' + i + '_cfg_parity'), 
        ms:getNumber('uart' + i + '_cfg_ms'), 
        ad:getNumber('uart' + i + '_cfg_addr')
    };

    var xhr = createXMLHttpRequest();
    if (xhr) {
        Show("正在设置串口参数,请稍后...");
		xhr.onreadystatechange = function() {
            if( xhr.readyState==4 ) {
                Close();
                if( xhr.status==200 ) {
                    var res = JSON.parse(xhr.responseText);
                    if( res && 0 == res.ret ) {
                        alert( "设置成功" );
                    } else {
                        alert("设置失败,请重试");
                    }
                } else {
                    alert("设置超时,请重试");
                }
            }
        }
		xhr.open("GET", "/cgi-bin/setUartCfg?arg=" + JSON.stringify(setval) );
		xhr.send();
	}
}

function Show(message){
    var shield = document.createElement("DIV");//产生一个背景遮罩层
    shield.id = "shield";
    shield.style.position = "absolute";
    shield.style.left = "0px";
    shield.style.top = "0px";
    shield.style.width = "100%";
    shield.style.height = ((document.documentElement.clientHeight>document.documentElement.scrollHeight)?document.documentElement.clientHeight:document.documentElement.scrollHeight)+"px";
    shield.style.background = "#333";
    shield.style.textAlign = "center";
    shield.style.zIndex = "10000";
    shield.style.filter = "alpha(opacity=0)";
    shield.style.opacity = 0;

    var alertFram = document.createElement("DIV");//产生一个提示框
    var height="40px";
    alertFram.id="alertFram";
    alertFram.style.position = "absolute";
    alertFram.style.width = "200px";
    alertFram.style.height = height;
    alertFram.style.left = "45%";
    alertFram.style.top = "50%";
    alertFram.style.background = "#fff";
    alertFram.style.textAlign = "center";
    alertFram.style.lineHeight = height;
    alertFram.style.zIndex = "100001";

   strHtml =" <div style=\"width:100%; border:#58a3cb solid 1px; text-align:center;\">";
    if (typeof(message)=="undefined"){
        strHtml+=" 正在操作, 请稍候...";
    } 
    else{
        strHtml+=message;
    }
    strHtml+=" </div>";

    alertFram.innerHTML=strHtml;
    document.body.appendChild(alertFram);
    document.body.appendChild(shield);

    this.setOpacity = function(obj,opacity){
        if(opacity>=1)opacity=opacity/100;
        try{ obj.style.opacity=opacity; }catch(e){}
        try{ 
            if(obj.filters.length>0&&obj.filters("alpha")){
            obj.filters("alpha").opacity=opacity*100;
            }else{
            obj.style.filter="alpha(opacity=\""+(opacity*100)+"\")";
            }
        }
        catch(e){}
    }

    var c = 0;
    this.doAlpha = function(){
    if (++c > 20){clearInterval(ad);return 0;}
    setOpacity(shield,c);
    }
    var ad = setInterval("doAlpha()",1);//渐变效果
    document.body.onselectstart = function(){return false;}
    document.body.oncontextmenu = function(){return false;}
}

function Close(){
    var shield= window.document.getElementById("shield");
    var alertFram= window.document.getElementById("alertFram");
    if(shield!=null) {
        document.body.removeChild(shield);
    }
    if(alertFram!=null) {
        document.body.removeChild(alertFram);
    } 
    document.body.onselectstart = function(){return true};
    document.body.oncontextmenu = function(){return true};
}

function getAllUartCfg()
{
    var xhr = createXMLHttpRequest();
    if (xhr) {
        Show("正在获取串口配置,请稍后...");
		xhr.onreadystatechange = function() {
            if( xhr.readyState==4 ) {
                Close();
                if( xhr.status==200 ) {
                    var res = JSON.parse(xhr.responseText);
                    if( res && 0 == res.ret ) {
                        xUartCfgList = res.list.concat();
                        for( var n = 0; n < res.list.length; n++ ) {
                            setUartHtml( 
                                n, 
                                res.list[n].bd, 
                                res.list[n].ut, 
                                res.list[n].po, 
                                res.list[n].py, 
                                res.list[n].ms, 
                                res.list[n].ad
                            );
                        }
                    } else {
                        alert("获取失败,请重试");
                    }
                } else {
                    alert("获取超时,请重试");
                }
            }
        }
		xhr.open("GET", "/cgi-bin/getUartCfg?arg={\"all\":1}" );
		xhr.send();
	}
}

function getUartCfg(i)
{
    var setval = { n:Number(i) };
    var xhr = createXMLHttpRequest();
    if (xhr) {
        Show("正在获取串口配置,请稍后...");
		xhr.onreadystatechange = function() {
            if( xhr.readyState==4 ) {
                Close();
                if( xhr.status==200 ) {
                    var res = JSON.parse(xhr.responseText);
                    if( res && 0 == res.ret ) {
                        setUartHtml( i, res.bd, res.ut, res.po, res.py, res.ms, res.ad );
                    } else {
                        alert("获取失败,请重试");
                    }
                } else {
                    alert("获取超时,请重试");
                }
            }
        }
		xhr.open("GET", "/cgi-bin/getUartCfg?arg=" + JSON.stringify(setval) );
		xhr.send();
	}
}

function refreshAllVarManageExtDataBase(index)
{
    refreshProtoDevList( xProtoDevList );
    varExtTableItemRemoveAll();
    for( var n = 0; n < xVarManageExtDataBase.length; n++ ) {
        varExtTableAddItem( 
            xVarManageExtDataBase[n].en, 
            xVarManageExtDataBase[n].na, 
            xVarManageExtDataBase[n].al, 
            xVarManageExtDataBase[n].vt, 
            xVarManageExtDataBase[n].vs, 
            xVarManageExtDataBase[n].it, 
            xVarManageExtDataBase[n].va, 
            xVarManageExtDataBase[n].dt, 
            xVarManageExtDataBase[n].pt
        );
    }
    var table = window.document.getElementById("rtu_var_ext_table");
    onExtTableItemClick(table, table.rows[index+1]);
}

function getAllVarManageExtDataBase()
{
    var xhr = createXMLHttpRequest();
    if (xhr) {
        Show("正在获取采集变量表,请稍后...");
		xhr.onreadystatechange = function() {
            if( xhr.readyState==4 ) {
                Close();
                if( xhr.status==200 ) {
                    var res = JSON.parse(xhr.responseText);
                    if( res && 0 == res.ret ) {
                        xVarManageExtDataBase = res.list.concat();
                        xProtoDevList = res.protolist;
                        refreshAllVarManageExtDataBase(0);
                    } else {
                        alert("获取失败,请重试");
                    }
                } else {
                    alert("获取超时,请重试");
                }
            }
        }
		xhr.open("GET", "/cgi-bin/getVarManageExtData?arg={\"all\":1}" );
		xhr.send();
	}
}

function getVarExtInfo()
{
    var obj = window.document.getElementById("var_ext_id");
    var id = -1;
    if( obj != null && obj.value.length > 0 ) {
        id = Number(obj.value);
    }
    
    if( id >= 0 && id <= 64 ) {
        var setval = { n:Number(id) };
        var xhr = createXMLHttpRequest();
        if (xhr) {
            Show("正在获取采集变量信息,请稍后...");
            xhr.onreadystatechange = function() {
                if( xhr.readyState==4 ) {
                    Close();
                    if( xhr.status==200 ) {
                        var res = JSON.parse(xhr.responseText);
                        if( res && 0 == res.ret ) {
                            if( xVarManageExtDataBase[id] != null ) {
                                xVarManageExtDataBase[id] = res;
                            }
                            var table = window.document.getElementById("rtu_var_ext_table");
                            onExtTableItemClick(table, table.rows[id]);
                        } else {
                            alert("获取失败,请重试");
                        }
                    } else {
                        alert("获取超时,请重试");
                    }
                }
            }
            xhr.open("GET", "/cgi-bin/getVarManageExtData?arg=" + JSON.stringify(setval) );
            xhr.send();
        }
    } else {
        alert("请先在列表中，选择要修改的选项，再进行读取");
    }
}

function varExtTableItemRemoveAll()
{
	var table = window.document.getElementById("rtu_var_ext_table");
    var rowNum = table.rows.length;
	if(rowNum > 1) {
		for(i=1;i<rowNum;i++) {
			table.deleteRow(i);
			rowNum = rowNum-1;
			i = i-1;
		}
	}
}

function onExtTableItemClick(tb,row)
{
    if( row.rowIndex > 0 ) {
        for (var i = 1; i < tb.rows.length; i++) {
            if( xVarManageExtDataBase[i-1].en > 0 ) {
                tb.rows[i].style.background="#F5F5F5";
            } else {
                tb.rows[i].style.background="#AAAAAA";
            }
        }
        row.style.background="#F007f0";

        setValue('var_ext_id', row.rowIndex-1);
        setValue('var_ext_enable', xVarManageExtDataBase[row.rowIndex-1].en);
        setValue('var_ext_name', xVarManageExtDataBase[row.rowIndex-1].na);
        setValue('var_ext_alias', xVarManageExtDataBase[row.rowIndex-1].al);
        setValue('var_ext_vartype', xVarManageExtDataBase[row.rowIndex-1].vt);
        setValue('var_ext_varsize', xVarManageExtDataBase[row.rowIndex-1].vs);
        setValue('var_ext_devtype', xVarManageExtDataBase[row.rowIndex-1].dt + "|" + xVarManageExtDataBase[row.rowIndex-1].pt);
        setValue('var_ext_addr', xVarManageExtDataBase[row.rowIndex-1].ad);
        setValue('var_ext_slaveaddr', xVarManageExtDataBase[row.rowIndex-1].sa);
        setValue('var_ext_extaddr', xVarManageExtDataBase[row.rowIndex-1].ea);
        setValue('var_ext_extaddrofs', xVarManageExtDataBase[row.rowIndex-1].ao);
        setValue('var_ext_varrw', xVarManageExtDataBase[row.rowIndex-1].rw);
        setValue('var_ext_interval', xVarManageExtDataBase[row.rowIndex-1].it);
        
        onVarExtVartypeChange( window.document.getElementById("var_ext_vartype") );
    }
}

function varExtGetProtoName(dev,proto)
{
    var str = "";
    switch(dev) {
    case 0: {
        switch(proto) {
        case 0: return "UART1_Modbus_RTU";
        }
        break;
    }
    case 1: {
        switch(proto) {
        case 0: return "UART2_Modbus_RTU";
        }
        break;
    }
    case 2: {
        switch(proto) {
        case 0: return "UART3_Modbus_RTU";
        }
        break;
    }
    case 3: {
        switch(proto) {
        case 0: return "UART4_Modbus_RTU";
        }
        break;
    }
    case 4: {
        switch(proto) {
        case 0: return "Net_Modbus_TCP_1";
        case 1: return "Net_Modbus_TCP_2";
        case 2: return "Net_Modbus_TCP_3";
        case 3: return "Net_Modbus_TCP_4";
        }
        break;
    }
    case 5: {
        switch(proto) {
        case 0: return "Zigbee_Modbus_RTU";
        }
        break;
    }
    case 6: {
        switch(proto) {
        case 0: return "GPRS_Modbus_TCP_1";
        case 1: return "GPRS_Modbus_TCP_2";
        case 2: return "GPRS_Modbus_TCP_3";
        case 3: return "GPRS_Modbus_TCP_4";
        }
        break;
    }
    }
    
    return "ERROR";
}

function varExtGetVarTypeName(type)
{
    switch(type){
    case 0: return "BIT";
    case 1: return "INT8";
    case 2: return "UINT8";
    case 3: return "INT16";
    case 4: return "UINT16";
    case 5: return "INT32";
    case 6: return "UINT32";
    case 7: return "FLOAT";
    case 8: return "DOUBLE";
    case 9: return "ARRAY";
    default: return "ERROR";
    }
}

function varExtTableAddItem(enable,name,alias,type,size,interval,value,protodev,proto)
{
	var table = window.document.getElementById("rtu_var_ext_table");
	var row = table.insertRow(table.rows.length);
    row.onclick = function(){ onExtTableItemClick( table, row ); };
    var obj = row.insertCell(0);
	obj.width = "10px"; obj.className = "tc";
    obj.innerHTML = table.rows.length - 1;
	obj = row.insertCell(1);
	obj.width = "10px"; obj.className = "tc";
    obj.innerHTML = enable != 0 ? "有效" : "无效";
	obj = row.insertCell(2);
	obj.width = "30px"; obj.className = "tc";
    obj.innerHTML = name;
	obj = row.insertCell(3);
	obj.width = "20px"; obj.className = "tc";
    obj.innerHTML = alias;
	obj = row.insertCell(4);
	obj.width = "10px"; obj.className = "tc";
    obj.innerHTML = varExtGetVarTypeName(type);
	obj = row.insertCell(5);
	obj.width = "10px"; obj.className = "tc";
    obj.innerHTML = size;
	obj = row.insertCell(6);
	obj.width = "20px"; obj.className = "tc";
    obj.innerHTML = enable != 0 ? interval : "--";
	obj = row.insertCell(7);
	obj.width = "35px"; obj.className = "tc";
    obj.innerHTML = enable != 0 ? value : "--";
	obj = row.insertCell(8);
	obj.width = "60px"; obj.className = "tc";
    obj.innerHTML = enable != 0 ? varExtGetProtoName(protodev, proto) : "--";
}

function onVarExtVartypeChange(obj)
{
    if( obj.selectedIndex < 3 ) {
        setValue("var_ext_varsize", 1 );
    } else if( obj.selectedIndex < 5 ) {
        setValue("var_ext_varsize", 2 );
    } else if( obj.selectedIndex < 8 ) {
        setValue("var_ext_varsize", 4 );
    } else if( obj.selectedIndex == 8 ) {
        setValue("var_ext_varsize", 8 );
    }
    
    if( obj.selectedIndex < 9 ) {
        setEnable("var_ext_varsize", false );
    } else {
        setEnable("var_ext_varsize", true );
    }
}

function setVarExtInfo()
{
    var obj = window.document.getElementById("var_ext_id");
    var id = -1;
    if( obj != null && obj.value.length > 0 ) {
        id = Number(obj.value);
    }
    
    if( id >= 0 && id <= 64 ) {
        
        onVarExtVartypeChange( window.document.getElementById("var_ext_vartype") );
        var varsize = getNumber('var_ext_varsize')
        if( varsize > 8 ) {
            alert("最大长度为8字节,请修改后重新保存");
            return ;
        }
        
        var addr = getNumber('var_ext_addr');

        if( addr < 1024 || addr > 2048 ) {
            alert("内部映射地址范围(1024->2048), 请修改后重新保存");
            return ;
        }
        
        var slaveaddr = getNumber('var_ext_slaveaddr');

        if( slaveaddr < 0 || slaveaddr > 255 ) {
            alert("Modbus从机地址范围(0->255), 请修改后重新保存");
            return ;
        }
            
        var proto = getValue("var_ext_devtype").split("|");
        var setval = {
            n:id, 
            en:getNumber('var_ext_enable'), 
            na:getValue('var_ext_name'), 
            al:getValue('var_ext_alias'), 
            vt:getNumber('var_ext_vartype'), 
            vs:getNumber('var_ext_varsize'), 
            dt:Number(proto[0]), 
            pt:Number(proto[1]), 
            ad:getNumber('var_ext_addr'), 
            sa:getNumber('var_ext_slaveaddr'), 
            ea:getNumber('var_ext_extaddr'), 
            ao:getNumber('var_ext_extaddrofs'), 
            rw:getNumber('var_ext_varrw'),
            it:getNumber('var_ext_interval')
        };
        
        xVarManageExtDataBase[id].en=setval.en;
        xVarManageExtDataBase[id].na=setval.na;
        xVarManageExtDataBase[id].al=setval.al;
        xVarManageExtDataBase[id].vt=setval.vt;
        xVarManageExtDataBase[id].vs=setval.vs;
        xVarManageExtDataBase[id].dt=setval.dt;
        xVarManageExtDataBase[id].pt=setval.pt;
        xVarManageExtDataBase[id].ad=setval.ad;
        xVarManageExtDataBase[id].sa=setval.sa;
        xVarManageExtDataBase[id].ea=setval.ea;
        xVarManageExtDataBase[id].ao=setval.ao;
        xVarManageExtDataBase[id].rw=setval.rw;
        xVarManageExtDataBase[id].it=setval.it;

        var xhr = createXMLHttpRequest();
        if (xhr) {
            Show("正在设置采集变量信息,请稍后...");
            xhr.onreadystatechange = function() {
                if( xhr.readyState==4 ) {
                    Close();
                    if( xhr.status==200 ) {
                        var res = JSON.parse(xhr.responseText);
                        if( res && 0 == res.ret ) {
                            refreshAllVarManageExtDataBase(id);
                            alert( "设置成功" );
                        } else {
                            alert("设置失败,请重试");
                        }
                    } else {
                        alert("设置超时,请重试");
                    }
                }
            }
            xhr.open("GET", "/cgi-bin/setVarManageExtData?arg=" + JSON.stringify(setval) );
            xhr.send();
        }
    } else {
        alert("请先在列表中，选择要修改的选项，再进行设置");
    }
}

function refreshProtoDevList(res)
{
    var protoDevList = window.document.getElementById("var_ext_devtype");
    protoDevList.options.length = 0;
    if( res.rs != null ) {
        for( var n = 0; n < res.rs.length; n++ ) {
            protoDevList.options.add(
                new Option(
                    varExtGetProtoName( res.rs[n].id, res.rs[n].po ), 
                    res.rs[n].id + "|" + res.rs[n].po
                )
            );
        }
    }
    if( res.net != null ) {
        for( var n = 0; n < res.net.length; n++ ) {
            protoDevList.options.add(
                new Option(
                    varExtGetProtoName( res.net[n].id, res.net[n].po ), 
                    res.net[n].id + "|" + res.net[n].po
                )
            );
        }
    }
    if( res.zigbee != null ) {
        for( var n = 0; n < res.zigbee.length; n++ ) {
            protoDevList.options.add(
                new Option(
                    varExtGetProtoName( res.zigbee[n].id, res.zigbee[n].po), 
                    res.zigbee[n].id + "|" + res.zigbee[n].po
                )
            );
        }
    }
    if( res.gprs != null ) {
        for( var n = 0; n < res.gprs.length; n++ ) {
            protoDevList.options.add(
                new Option(
                    varExtGetProtoName( res.gprs[n].id, res.gprs[n].po), 
                    res.gprs[n].id + "|" + res.gprs[n].po
                )
            );
        }
    }
}

function getProtoDevList()
{
    var xhr = createXMLHttpRequest();
    if (xhr) {
        Show("正在获取驱动协议列表,请稍后...");
		xhr.onreadystatechange = function() {
            if( xhr.readyState==4 ) {
                Close();
                if( xhr.status==200 ) {
                    var res = JSON.parse(xhr.responseText);
                    if( res && 0 == res.ret ) {
                        xProtoDevList = res;
                        refreshProtoDevList( res );
                    } else {
                        alert("获取驱动协议列表失败,请重试");
                    }
                } else {
                    alert("获取驱动协议列表超时,请重试");
                }
            }
        }
        xhr.open("GET", "/cgi-bin/getProtoManage?" );
        xhr.send();
	}
}

function refreshDevInfo(info)
{
    setValue('rtu_dev_id', info.id);
    setValue('rtu_dev_hw_ver', Number(Number(info.hw) / 100.0).toFixed(2));
    setValue('rtu_dev_sw_ver', Number(Number(info.sw) / 100.0).toFixed(2));
    setValue('rtu_dev_oem', info.om);
    setValue('rtu_dev_mac', info.mc);
    setValue('rtu_dev_time', info.dt);
}

function getDevInfo()
{
    var xhr = createXMLHttpRequest();
    if (xhr) {
        Show("正在获取设备信息,请稍后...");
		xhr.onreadystatechange = function() {
            if( xhr.readyState==4 ) {
                Close();
                if( xhr.status==200 ) {
                    var res = JSON.parse(xhr.responseText);
                    if( res && 0 == res.ret ) {
                        refreshDevInfo( res );
                    } else {
                        alert("获取设备信息失败,请重试");
                    }
                } else {
                    alert("获取设备信息超时,请重试");
                }
            }
        }
        xhr.open("GET", "/cgi-bin/getDevInfo?" );
        xhr.send();
	}
}

function devTimeSync()
{
    var myDate = new Date();
    var setval = {
        ye:myDate.getFullYear(), 
        mo:(myDate.getMonth()+1), 
        da:myDate.getDate(), 
        dh:myDate.getHours(), 
        hm:myDate.getMinutes(), 
        ms:myDate.getSeconds()
    };
    
    MyGetJSON("正在对时,请稍后...","/cgi-bin/setTime?","arg=" + JSON.stringify(setval));
}

function refreshNetInfo(info)
{
    /*setValue('rtu_dev_id', info.id);
    setValue('rtu_dev_hw_ver', Number(Number(info.hw) / 100.0).toFixed(2));
    setValue('rtu_dev_sw_ver', Number(Number(info.sw) / 100.0).toFixed(2));
    setValue('rtu_dev_oem', info.om);
    setValue('rtu_dev_mac', info.mc);
    setValue('rtu_dev_time', info.dt);*/
    
    if( info.status != null ) {
        window.document.getElementById("lbl_netStatus").innerHTML=info.status;
    }
}

function getNetInfo()
{
    var xhr = createXMLHttpRequest();
    if (xhr) {
        Show("正在获取网络信息,请稍后...");
		xhr.onreadystatechange = function() {
            if( xhr.readyState==4 ) {
                Close();
                if( xhr.status==200 ) {
                    var res = JSON.parse(xhr.responseText);
                    if( res && 0 == res.ret ) {
                        refreshNetInfo( res );
                    } else {
                        alert("获取网络信息失败,请重试");
                    }
                } else {
                    alert("获取网络信息超时,请重试");
                }
            }
        }
        xhr.open("GET", "/cgi-bin/getNetCfg?" );
        xhr.send();
	}
}

function getAllTcpipCfg()
{
    var xhr = createXMLHttpRequest();
    if (xhr) {
        Show("正在获取TCP/IP配置,请稍后...");
		xhr.onreadystatechange = function() {
            if( xhr.readyState==4 ) {
                Close();
                if( xhr.status==200 ) {
                    var res = JSON.parse(xhr.responseText);
                    if( res && 0 == res.ret ) {
                        xTcpipCfgList = res.list.concat();
                        for( var n = 0; n < res.list.length; n++ ) {
                            refreshTcpipCfg( 
                                n, 
                                res.list[n]
                            );
                        }
                    } else {
                        alert("获取TCP/IP配置失败,请重试");
                    }
                } else {
                    alert("获取TCP/IP配置超时,请重试");
                }
            }
        }
        xhr.open("GET", "/cgi-bin/getTcpipCfg?arg={\"all\":1}" );
        xhr.send();
    }
}

function getTcpipCfg(i)
{
    var setval = { n:Number(i) };
    var xhr = createXMLHttpRequest();
    if (xhr) {
        Show("正在获取TCP/IP配置,请稍后...");
		xhr.onreadystatechange = function() {
            if( xhr.readyState==4 ) {
                Close();
                if( xhr.status==200 ) {
                    var res = JSON.parse(xhr.responseText);
                    if( res && 0 == res.ret ) {
                        refreshTcpipCfg( i, res );
                    } else {
                        alert("获取TCP/IP配置失败,请重试");
                    }
                } else {
                    alert("获取TCP/IP配置超时,请重试");
                }
            }
        }
        xhr.open("GET", "/cgi-bin/getTcpipCfg?arg=" + JSON.stringify(setval) );
        xhr.send();
    }
}

function refreshTcpipCfg(n,cfg)
{
    var prefix = "net_tcpip";
    if(n >= 4) {
        prefix = "gprs_tcpip";
        n -= 4;
    }
    
    setValue(prefix+n+'_enable', cfg.en);
    setValue(prefix+n+'_type', cfg.tt);
    setValue(prefix+n+'_cs', cfg.cs);
    setValue(prefix+n+'_proto', cfg.pt);
    setValue(prefix+n+'_ms', cfg.ms);
    setValue(prefix+n+'_peer', cfg.pe);
    setValue(prefix+n+'_port', cfg.po);
}

function setTcpipCfg(i)
{    
    var n = i;
    var prefix = "net_tcpip";
    if(i >= 4) {
        prefix = "gprs_tcpip";
        i -= 4;
    }
    var setval = {
        n:Number(n), 
        en:getNumber(prefix + i + '_enable'), 
        tt:getNumber(prefix + i + '_type'), 
        pt:getNumber(prefix + i + '_proto'), 
        cs:getNumber(prefix + i + '_cs'), 
        ms:getNumber(prefix + i + '_ms'), 
        pe:getValue(prefix + i + '_peer'), 
        po:getNumber(prefix + i + '_port')
    };
    
    var xhr = createXMLHttpRequest();
    if (xhr) {
        Show("正在配置TCP/IP,请稍后...");
		xhr.onreadystatechange = function() {
            if( xhr.readyState==4 ) {
                Close();
                if( xhr.status==200 ) {
                    var res = JSON.parse(xhr.responseText);
                    if( res && 0 == res.ret ) {
                        alert("配置TCP/IP成功");
                    } else {
                        alert("配置TCP/IP失败,请重试");
                    }
                } else {
                    alert("配置TCP/IP超时,请重试");
                }
            }
        }
        xhr.open("GET", "/cgi-bin/setTcpipCfg?arg=" + JSON.stringify(setval) );
        xhr.send();
    }
}


function getDOValue(i)
{
    if( i < 4 ) {
        return getValue('btn_writeDO_'+i) == "  开  "?1:0;
    } else {
        return getNumber('dido_do_' + i);
    }
}

function setDOValue(i, val)
{
    if( i < 4 ) {
        setValue('btn_writeDO_'+i, val!=0?"  关  ":"  开  ");
    } else {
        setValue( "dido_do_"+i, val );
    }
}

function readDIDO()
{
    var xhr = createXMLHttpRequest();
    if (xhr) {
        Show("正在读取输入输出模块信息,请稍后...");
		xhr.onreadystatechange = function() {
            if( xhr.readyState==4 ) {
                Close();
                if( xhr.status==200 ) {
                    var res = JSON.parse(xhr.responseText);
                    if( res && 0 == res.ret ) {
                        for( var i = 0; i < res.di.length; i++ ) {
                            setValue( "dido_di_"+i, res.di[i].va );
                        }
                        for( var i = 0; i < res.do.length; i++ ) {
                            setDOValue( i, res.do[i].va );
                        }
                    } else {
                        alert("获取失败,请重试");
                    }
                } else {
                    alert("获取超时,请重试");
                }
            }
        }
        xhr.open("GET", "/cgi-bin/readDIDO?" );
        xhr.send();
    }
}

function writeDO(i)
{
    var setval = {
        n:Number(i), 
        va:getDOValue(i)
    };
    
    var xhr = createXMLHttpRequest();
    if (xhr) {
        Show("正在设置输出模块信息,请稍后...");
		xhr.onreadystatechange = function() {
            if( xhr.readyState==4 ) {
                Close();
                if( xhr.status==200 ) {
                    var res = JSON.parse(xhr.responseText);
                    if( res && 0 == res.ret ) {
                        for( var i = 0; i < res.di.length; i++ ) {
                            setValue( "dido_di_"+i, res.di[i].va );
                        }
                        for( var i = 0; i < res.do.length; i++ ) {
                            setDOValue( i, res.do[i].va );
                        }
                        alert("设置成功");
                    } else {
                        alert("设置失败,请重试");
                    }
                } else {
                    alert("设置超时,请重试");
                }
            }
        }
        xhr.open("GET", "/cgi-bin/writeDO?arg=" + JSON.stringify(setval) );
        xhr.send();
    }
}

function onAiTypeChange(n)
{
    setVisible("ai_extval_set_"+n, getNumber("ai_type_"+n)==3);
}

function setAICfg(n)
{
    var setval = {
        n:Number(n), 
        rg:getNumber('ai_range_' + n), 
        ut:getNumber('ai_type_' + n), 
        ei:getNumber('ai_extval_min_' + n), 
        ea:getNumber('ai_extval_max_' + n), 
        ec:getNumber('ai_extval_corr_' + n)
    };
    
    var xhr = createXMLHttpRequest();
    if (xhr) {
        Show("正在设置通道"+(n+1)+",请稍后...");
		xhr.onreadystatechange = function() {
            if( xhr.readyState==4 ) {
                Close();
                if( xhr.status==200 ) {
                    var res = JSON.parse(xhr.responseText);
                    if( res && 0 == res.ret ) {
                        alert("设置成功");
                    } else {
                        alert("设置失败,请重试");
                    }
                } else {
                    alert("设置超时,请重试");
                }
            }
        }
        xhr.open("GET", "/cgi-bin/setAnalogCfg?arg=" + JSON.stringify(setval) );
        xhr.send();
    }
}

function refreshAICfg(n,cfg)
{
    if( cfg != null ) {
        setValue( "ai_range_"+n, cfg.rg );
        setValue( "ai_type_"+n, cfg.ut );
        setValue( "ai_extval_min_"+n, cfg.ei );
        setValue( "ai_extval_max_"+n, cfg.ea );
        setValue( "ai_extval_corr_"+n, cfg.ec );
        setValue( "lbl_ai_val_"+n, cfg.va );
        var obj = window.document.getElementById("lbl_ai_val_"+n);
        if( obj != null && cfg.va != null ) {
            obj.innerHTML = "当前值:" + cfg.va;
        }
    }
    onAiTypeChange(n);
}

function getAllAICfg()
{
    var xhr = createXMLHttpRequest();
    if (xhr) {
        Show("正在读取模拟量配置,请稍后...");
		xhr.onreadystatechange = function() {
            if( xhr.readyState==4 ) {
                Close();
                if( xhr.status==200 ) {
                    var res = JSON.parse(xhr.responseText);
                    if( res && 0 == res.ret ) {
                        for( var i = 0; i < res.list.length; i++ ) {
                            refreshAICfg( i, res.list[i] );
                        }
                    } else {
                        alert("获取失败,请重试");
                    }
                } else {
                    alert("获取超时,请重试");
                }
            }
        }
        xhr.open("GET", "/cgi-bin/getAnalogCfg?arg={\"all\":1}" );
        xhr.send();
    }
}

function refreshGPRSInfo(info)
{
    setValue( "gprs_info_apn", info.apn );
    setValue( "gprs_info_user", info.user );
    setValue( "gprs_info_psk", info.psk );
    
    if( info.status != null ) {
        window.document.getElementById("lbl_GPRSState").innerHTML=info.status;
    }
}

function getGPRSInfo()
{
    var xhr = createXMLHttpRequest();
    if (xhr) {
        Show("正在读取GPRS配置,请稍后...");
		xhr.onreadystatechange = function() {
            if( xhr.readyState==4 ) {
                Close();
                if( xhr.status==200 ) {
                    var res = JSON.parse(xhr.responseText);
                    if( res && 0 == res.ret ) {
                        refreshGPRSInfo(res);
                    } else {
                        alert("读取失败,请重试");
                    }
                } else {
                    alert("读取超时,请重试");
                }
            }
        }
        xhr.open("GET", "/cgi-bin/getGPRSCfg?" );
        xhr.send();
    }
}

function setGPRSInfo()
{
    var setval = {
        apn:getValue("gprs_info_apn"), 
        user:getValue("gprs_info_user"), 
        psk:getValue("gprs_info_psk"),
    };
    
    var xhr = createXMLHttpRequest();
    if (xhr) {
        Show("正在设置GPRS模块,请稍后...");
		xhr.onreadystatechange = function() {
            if( xhr.readyState==4 ) {
                Close();
                if( xhr.status==200 ) {
                    var res = JSON.parse(xhr.responseText);
                    if( res && 0 == res.ret ) {
                        alert("设置成功");
                    } else {
                        alert("设置失败,请重试");
                    }
                } else {
                    alert("设置超时,请重试");
                }
            }
        }
        xhr.open("GET", "/cgi-bin/setGPRSCfg?arg=" + JSON.stringify(setval) );
        xhr.send();
    }  
}


//======================================================页面js函数==================================================
//每次选择菜单时，都会填写【功能名称】，待调用ak47时使用；
function bullet(name)
{
    document.getElementById("txt_refresh_name").value = name;
}

function bullet_clear()
{
    document.getElementById("txt_refresh_name").value = "";
}

function bullet_add(name) 
{
    document.getElementById("txt_refresh_name").value = name;
}


function msg()
{
    alert('应用成功！');
}

function chk(obj,name)
{
    if (obj == "" || obj == "undefined")
    {
        alert("【" + name + "】不能为空！");
        return true;
    }
    else
    {
        return false;
    }
}

function chk2(obj1, obj2, name)
{
    if ((obj1 == "" || obj1 == "undefined") && (obj2 == "" || obj2 == "undefined"))
    {
        alert("【" + name + "】不能为空！");
        return true;
    }
    else
    {
        return false;
    }
}

var v = 0;
//自动刷新时，调用该函数；
function ak47()
{
    var name = document.getElementById("txt_refresh_name").value;

    switch (name)
    {
        case "1001":
            {
                //设备基本信息
                Show_Basic_Information();
            }
            break;
        case "1002":
            {
                //系统资源
                Show_System_Resource();
            }
            break;
        case "1003":
            {
                //GPRS连接状态
                Show_Root_GPRS_Network();
                //以太网连接状态
                //Show_Root_Network();
                //GPRS网络信息
                Show_gprs_network();
                //网络信息
                Show_Network_Information();
            }
            break;
        case "1004":
            {
                //已连接无线节点
                Show_Root_Information();
            }
            break;
        case "1005":
            {
                //基本设置
                Show_Time_Synchronization();
                //仅刷新一次；
                bullet_clear();
            }
            break;
        case "1006":
            {
                //日志
                Show_Log();
                //仅刷新一次；
                bullet_clear();
            }
            break;
        case "1007":
            {
                //管理权限
                Show_Jurisdiction();
                //仅刷新一次；
                bullet_clear();
            }
            break;
        case "1008":
            {
                //远程传输设置
                Show_Remote_Transmission();
                //仅刷新一次；
                bullet_clear();
            }
            break;
        case "1009":
            {
                //本地网络配置
                Show_Local_Network_Configuration();
                //仅刷新一次；
                bullet_clear();
            }
            break;
        case "1010":
            {
                //ZIGBEE配置
                Show_ZIGBEE();
                //仅刷新一次；
                bullet_clear();
            }
            break;
        case "1011":
            {
                //LORA配置
                Show_LORA();
                //仅刷新一次；
                bullet_clear();
            }
            break;
        case "1012":
            {
                //工作模式及中心配置
                Show_GPRS_Transmission();
                //仅刷新一次；
                bullet_clear();
            }
            break;
        case "1013":
            {
                //GPRS工作参数配置
                Show_GPRS_Config();
                //仅刷新一次；
                bullet_clear();
            }
            break;
        case "1014":
            {
                //无线参数
                Show_GPRS_Wireless();
                //仅刷新一次；
                bullet_clear();
            }
            break;
        case "1015":
            {
                //串口配置
                Show_Serial_Port();
                //仅刷新一次；
                bullet_clear();
            }
            break;
        case "1016":
            {
                //开关量输入设置
                Show_Switch_Cfg();
                //跳转
                bullet_add("9016");
            }
            break;
        case "9016":
            {
                Show_Switch_Value();
            }
            break;
        case "1017":
            {
                //开关量输出设置
                Show_Output_Cfg();
                Show_Output_Value();
                //仅刷新一次；
                bullet_clear();
            }
            break;
        case "1018":
            {
                //模拟量输入设置
                Show_Input();
                //跳转
                bullet_add("9018");
            }
            break;

        case "9018":
            {
                Show_Input_Range_Electrical();
            }
            break;
        case "1020":
            {
                //脚本执行器
            }
            break;
        case "1021":
            {
                //采集变量
                getAllVarManageExtDataBase();
                //仅刷新一次；
                bullet_clear();
            }
            break;
        case "1022":
            {
                //用户变量
            }
            break;

        case "1023":
            {
                //动作
            }
            break;
            
        default: break;
    }

}

//设备基本信息
function Show_Basic_Information()
{
    //*注：在此处编写获取代码；然而调用如下函数，实现页面显示；
    MyGetJSON("","/cgi-bin/getDevInfo?","", function (info)
    {
        Page_Basic_Information(info)
    });
}

function Page_Basic_Information( info )
{
    //设备ID
    document.getElementById("rtu_dev_id").innerHTML = info.id;
    //硬件版本
    document.getElementById("rtu_dev_hw_ver").innerHTML = Number(Number(info.hw) / 100.0).toFixed(2);
    //固件版本
    document.getElementById("rtu_dev_sw_ver").innerHTML = Number(Number(info.sw) / 100.0).toFixed(2);
    //产品序列号
    document.getElementById("rtu_dev_oem").innerHTML = info.om;
    //Zigbee 固件版本
    //document.getElementById("rtu_dev_oem").innerHTML = info.om;
    //MAC 地址
    document.getElementById("rtu_dev_mac").innerHTML = info.mc;
    //系统时间
    document.getElementById("rtu_dev_time").innerHTML = info.dt;
    //运行时间
    document.getElementById("rtu_run_time").innerHTML = info.rt;
}

//系统资源
function Show_System_Resource()
{
    //*注：在此处编写获取代码；然而调用如下函数，实现页面显示；
    //Page_System_Resource("注：此处传入一个对象");
    MyGetJSON("","/cgi-bin/getCpuUsage?","", function (info)
    {
        Page_System_Resource(info)
    });
}

function Page_System_Resource(info)
{
    //CPU使用率
    document.getElementById("sys_cpu_rate").innerHTML = Number(info.cpu) + "%";
    //内存占用
    if( Number(info.ms) > 0 ) {
        document.getElementById("sys_memory_use").innerHTML = Math.round(100*Number(info.mu)/Number(info.ms)) + "%";
    } else {
        document.getElementById("sys_memory_use").innerHTML = "--";
    }
    //内存剩余
    document.getElementById("sys_memory_free").innerHTML = (Number(info.ms)-Number(info.mu)) + " Byte";
    //存储ROM
    document.getElementById("sys_store_rom").innerHTML = (Number(info.fs)-Number(info.fu)) + " KB";
    //SD卡
    document.getElementById("sys_store_sd").innerHTML = (Number(info.ss)-Number(info.su)) + " KB";
}

//网络信息
function Show_Network_Information()
{
    //*注：在此处编写获取代码；然而调用如下函数，实现页面显示；
    MyGetJSON("","/cgi-bin/getNetInfo?","", function (info)
    {
        Page_Network_Information(info)
    });
}

function Page_Network_Information(info)
{
    //类型: dhcp
    document.getElementById("sys_net_dhcp").innerHTML = Number(info.dh)!=0?"开":"关";
    //MAC地址
    document.getElementById("sys_net_mac").innerHTML = info.mac;
    //地址
    document.getElementById("sys_net_address").innerHTML = info.ip;
    //子网掩码
    document.getElementById("sys_net_mask").innerHTML = info.mask;
    //网关
    document.getElementById("sys_net_gate").innerHTML = info.gw;
    //DNS 1
    document.getElementById("sys_net_dns_1").innerHTML = info.d1;
    //DNS 2
    document.getElementById("sys_net_dns_2").innerHTML = info.d2;
    //已连接
    document.getElementById("sys_net_link").innerHTML = info.lk;
}

//已连接无线节点
function Show_Root_Information()
{
    //*注：在此处编写获取代码；然而调用如下函数，实现页面显示；
    //Page_Root_Information("注：传入【二维数据】！！！！！！！");
}

function Page_Root_Information(obj)
{
    var str = "<table>";
    str += "<tr><td>主机名</td><td>网络地址</td><td>设备地址</td><td>MAC地址</td><td>信号强度</td><td>噪声</td><td>速率</td><td>连接时长</td></tr>";

    try
    {
        for (var n = 0; n < obj.length; n++)
        {
            str += "<tr>";
            for (var j = 0; j < obj[n].length; j++)
            {
                str += "<td>" + obj[n][j] + "</td>";
            }
            str += "</tr>";
        }
    }
    catch(e)
    {
    }

    str += "</table>";

    document.getElementById("div_root").innerHTML = str;

}

//GPRS网络信息
function Show_gprs_network()
{
    //*注：在此处编写获取代码；然而调用如下函数，实现页面显示；
    //Page_gprs_network("")
    MyGetJSON("","/cgi-bin/getGPRSState?","", function (info)
    {
        Page_gprs_network(info)
    });
}

function Page_gprs_network(info)
{
    //运营商名称
    document.getElementById("gprs_network_name").innerHTML = info.alphan;
    //网络注册状态
    document.getElementById("gprs_network_creg").innerHTML = info.creg;
    //信号质量
    document.getElementById("gprs_network_csq").innerHTML = info.csq;
    //小区信息
    document.getElementById("gprs_network_area").innerHTML = info.area;
}



//GPRS 与 以太网 连接状态
function Show_Root_GPRS_Network()
{
    //*注：在此处编写获取代码；然而调用如下函数，实现页面显示；
    //Page_Root_GPRS("12345");
    MyGetJSON("","/cgi-bin/getTcpipState?","", function (info)
    {
        Page_Root_GPRS(info.states);
        Page_Root_Network(info.states);
    });
}

function Page_Root_GPRS(obj)
{
    var str = "<table style='width:96%'>";
    str += "<tr><td style='background-color:#8FC6FC'>序号</td><td style='background-color:#8FC6FC'>远程IP</td><td style='background-color:#8FC6FC'>远程端口</td><td style='background-color:#8FC6FC'>本地IP</td><td style='background-color:#8FC6FC'>本地端口</td><td style='background-color:#8FC6FC'>状态</td><td style='background-color:#8FC6FC'>已连接:（时长）</td></tr>";

    try
    {
        for (var n = 4; n < 8; n++)
        {
            str += "<tr>";
            str += "<td>" + (n-4+1) + "</td>";
            if( 0 == obj[n].en ) {
                str += "<td>--</td>";
                str += "<td>--</td>";
                str += "<td>--</td>";
                str += "<td>--</td>";
                str += "<td>未启用</td>";
                str += "<td>--</td>";
            } else {
                str += "<td>" + obj[n].rip + "</td>";
                str += "<td>" + obj[n].rpt + "</td>";
                str += "<td>" + obj[n].lip + "</td>";
                str += "<td>" + obj[n].lpt + "</td>";
                switch(obj[n].st) {
                case 0:
                    str += "<td>等待</td>";
                    break;
                case 1:
                    str += "<td>已连接</td>";
                    break;
                case 2:
                    str += "<td>正在连接</td>";
                    break;
                case 3:
                    str += "<td>等待连接</td>";
                    break;
                }
                if( 1 == obj[n].st ) {
                    str += "<td>" + (obj[n].tn-obj[n].tc) + " 秒</td>";
                } else {
                    str += "<td>--</td>";
                }
            }
            str += "</tr>";
        }
    }
    catch (e)
    {
    }

    str += "</table>";

    document.getElementById("div_gprs").innerHTML = str;

}


//GPRS连接状态
//function Show_Root_Network()
//{
//    //*注：在此处编写获取代码；然而调用如下函数，实现页面显示；
//    Page_Root_Network("12345");
//}

function Page_Root_Network(obj)
{
    var str = "<table style='width:96%'>";
    str += "<tr><td style='background-color:#8FC6FC'>序号</td><td style='background-color:#8FC6FC'>远程IP</td><td style='background-color:#8FC6FC'>远程端口</td><td style='background-color:#8FC6FC'>本地IP</td><td style='background-color:#8FC6FC'>本地端口</td><td style='background-color:#8FC6FC'>状态</td><td style='background-color:#8FC6FC'>已连接:（时长）</td></tr>";

    try
    {
        for (var n = 0; n < 4; n++)
        {
            str += "<tr>";
            str += "<td>" + (n+1) + "</td>";
            if( 0 == obj[n].en ) {
                str += "<td>--</td>";
                str += "<td>--</td>";
                str += "<td>--</td>";
                str += "<td>--</td>";
                str += "<td>未启用</td>";
                str += "<td>--</td>";
            } else {
                str += "<td>" + obj[n].rip + "</td>";
                str += "<td>" + obj[n].rpt + "</td>";
                str += "<td>" + obj[n].lip + "</td>";
                str += "<td>" + obj[n].lpt + "</td>";
                switch(obj[n].st) {
                case 0:
                    str += "<td>等待</td>";
                    break;
                case 1:
                    str += "<td>已连接</td>";
                    break;
                case 2:
                    str += "<td>正在连接</td>";
                    break;
                case 3:
                    str += "<td>等待连接</td>";
                    break;
                }
                if( 1 == obj[n].st ) {
                    str += "<td>" + (obj[n].tn-obj[n].tc) + " 秒</td>";
                } else {
                    str += "<td>--</td>";
                }
            }
            str += "</tr>";
        }
    }
    catch (e)
    {
    }

    str += "</table>";

    document.getElementById("div_network").innerHTML = str;

}

//时间同步
function Show_Time_Synchronization()
{
    //*注：在此处编写获取代码；然而调用如下函数，实现页面显示；
    Page_Time_Synchronization("注：此处传入一个对象");
}

function Page_Time_Synchronization(info)
{
    //主机名
    setValue('base_hostname', info.hn);
    //时间同步
    setValue('base_time_yes', Number(info.tn)!=0);
    setValue('base_time_no', !(Number(info.tn)!=0));
    //候选NTP服务器1
    setValue('base_ntp1', info.nt1);
    //候选NTP服务器2
    setValue('base_ntp2', info.nt2);
}

function Apply_Time_Synchronization()
{
    //主机名
    var base_hostname = document.getElementById("base_hostname").value;
    //时间同步
    var base_time1 = document.getElementsByName("base_time")[0].checked;
    var base_time2 = document.getElementsByName("base_time")[1].checked;
    //候选NTP服务器1
    var base_ntp1 = document.getElementById("base_ntp1").value;
    //候选NTP服务器2
    var base_ntp2 = document.getElementById("base_ntp2").value;

    if (chk(base_hostname, "主机名")) return;
    if (chk2(base_time1, base_time2, "时间同步")) return;
    if (chk(base_ntp1, "候选NTP服务器1")) return;
    if (chk(base_ntp2, "候选NTP服务器2")) return;

    //注：在此处填写保存代码即可；
    msg();
}

//日志
function Show_Log()
{
    //*注：在此处编写获取代码；然而调用如下函数，实现页面显示；
    Page_Log("注：此处传入一个对象");
}

function Page_Log(obj)
{
    //系统日志缓冲区大小 
    setValue('log_size', obj.null);
    //远程log服务器 
    setValue('log_remote', obj.null);
    //远程log服务器端口
    setValue('log_port', obj.null);

    //日志记录等级
    var tmp_log_level = document.getElementById("log_level");
    for (var i = 0; i < tmp_log_level.options.length; i++)
    {
        if (tmp_log_level.options[i].value == "此处填写对象.值")
        {
            tmp_log_level.selectedIndex = i;
            break;
        }
    }

    //Cron日志级别 
    var log_cron = document.getElementById("log_cron");
    for (var i = 0; i < log_cron.options.length; i++)
    {
        if (log_cron.options[i].value == "此处填写对象.值")
        {
            log_cron.selectedIndex = i;
            break;
        }
    }
}

function Apply_Log()
{
    //系统日志缓冲区大小
    var log_size = document.getElementById("log_size").value;
    //远程log服务器 
    var log_remote = document.getElementById("log_remote").value;
    //远程log服务器端口
    var log_port = document.getElementById("log_port").value;
    //日志记录等级
    var v = document.getElementById("log_level").selectedIndex;
    var log_level_ = document.getElementById("log_level").options[v].text;

    //Cron日志级别
    v = document.getElementById("log_cron").selectedIndex;
    var log_cron_ = document.getElementById("log_cron").options[v].text;


    if (chk(log_size, "系统日志缓冲区大小")) return;
    if (chk(log_remote, "远程log服务器")) return;
    if (chk(log_port, "远程log服务器端口")) return;
    if (chk(log_level_, "日志记录等级")) return;
    if (chk(log_cron_, "Cron日志级别")) return;

    //注：在此处填写保存代码即可；
    msg();
}


//管理权限
function Show_Jurisdiction()
{
    //*注：在此处编写获取代码；然而调用如下函数，实现页面显示；
    Page_Jurisdiction("注：此处传入一个对象");
}

function Page_Jurisdiction(obj)
{
    //用户名 
    setValue('manage_user', obj.null);
    //密码
    setValue('manage_pwd', obj.null);
    //SSH访问
    document.getElementsByName("manage_ssh")[0].checked = obj.null;//此处填写值；
    document.getElementsByName("manage_ssh")[1].checked = !obj.null;//此处填写值；
    //端口
    setValue('manage_port', obj.null);
    //允许SSH密码验证
    document.getElementsByName("manage_ssh_cer")[0].checked = obj.null;//此处填写值；
    document.getElementsByName("manage_ssh_cer")[1].checked = !obj.null;//此处填写值；
    //时间设置 
    setValue('manage_time', obj.null);
}


function Apply_Jurisdiction()
{
    //用户名
    var manage_user = document.getElementById("manage_user").value;
    //密码 
    var manage_pwd = document.getElementById("manage_pwd").value;
    //SSH访问
    var manage_ssh1 = document.getElementsByName("manage_ssh")[0].checked;
    var manage_ssh2 = document.getElementsByName("manage_ssh")[1].checked;
    //端口
    var manage_port = document.getElementById("manage_port").value;
    //允许SSH密码验证
    var manage_ssh_cer1 = document.getElementsByName("manage_ssh_cer")[0].checked;
    var manage_ssh_cer2 = document.getElementsByName("manage_ssh_cer")[1].checked;
    //时间设置
    var manage_time = document.getElementById("manage_time").value;


    if (chk(manage_user, "用户名")) return;
    if (chk(manage_pwd, "密码")) return;
    if (chk2(manage_ssh1, manage_ssh2, "SSH访问")) return;
    if (chk(manage_port, "端口")) return;
    if (chk2(manage_ssh_cer1, manage_ssh_cer2, "允许SSH密码验证")) return;
    if (chk(manage_time, "时间设置")) return;

    //注：在此处填写保存代码即可；
    msg();
}


function Show_Local_Setup()
{
    var v = Number(document.getElementById("local_dhcp").value);
    document.getElementById("local_ip").disabled = (v>0);
    document.getElementById("local_mask").disabled = (v>0);
    document.getElementById("local_gateway").disabled = (v>0);
    document.getElementById("local_dns").disabled = (v>0);
}

//远程传输设置
function Show_Remote_Transmission()
{
    //每次选择时，都会调用该函数；
    //获取【远程传输参数集】；=====================================================================>需要调用【设备接口】获取
    //如下是例子，例如：
    //Tabel_Remote_Parameter(new Array(new Array("AAA", "BBB", "CCC", "DDD", "EEE", "FFF", "GGG", "HHH", "JJJ"), new Array("KKK", "LLL", "MMM", "OOO", "PPP", "QQQ", "RRR", "SSS", "TTT"), new Array("UUU", "VVV", "WWW")));
    //-----------------------------------------------------------------------------------------------------------------------

    var v = document.getElementById("remote_group").selectedIndex;
    var setval = { n:Number(v) };
    $.getJSON("/cgi-bin/getTcpipCfg?arg="+JSON.stringify(setval),function( info ){
        Page_Remote_Transmission(info)
    });
}

var Remote_Parameter_Items = 0;
function Tabel_Remote_Parameter(obj)
{
    var str = "<table id='table_remote_upload' style='display:none'>";

    try
    {
        var k = 0;
        for (var n = 0; n < obj.length; n++)
        {
            str += "<tr>";
            for (var j = 0; j < obj[n].length; j++)
            {
                str += "<td><input name='remote_upload_data' type='checkbox' id='remote_upload_data" + k + "' value='" + k + "' />&nbsp;参数" + obj[n][j] + "</td>";
                k++;
            }
            str += "</tr>";
        }
        Remote_Parameter_Items = k;
    }
    catch (e)
    {
    }

    str += "</table>";

    document.getElementById("div_remote_parameter").innerHTML = str;

}

function Page_Remote_Transmission(cfg)
{

    //是否启用 
    document.getElementsByName("remote_group_is")[0].checked = (Number(cfg.en)!=0);
    document.getElementsByName("remote_group_is")[1].checked = !(Number(cfg.en)!=0);
    //以太网工作模式
    setValue('remote_working_type', cfg.tt);
    setValue('remote_working_cs', cfg.cs);
    //协议
    setValue('remote_proto', cfg.pt);
    setValue('remote_proto_ms', cfg.ms);
    //设备NM号（0-20位）
    //setValue('remote_dev', obj.null);
    //域名地址
    setValue('remote_ip', cfg.pe);
    //域名端口
    setValue('remote_port', cfg.po);
    //上传间隔
    setValue('remote_upload_interval', cfg.it);

    //上传的数据; =====================================================================>千万注意此处，绑定方式！！
    /*for (var i = 0; i < Remote_Parameter_Items; i++)
    {
        document.getElementById("remote_upload_data" + i).checked = true;
    }*/

    //是否启用心跳包 
    document.getElementsByName("remote_heartbeat")[0].checked = (Number(cfg.kl)!=0);
    document.getElementsByName("remote_heartbeat")[1].checked = !(Number(cfg.kl)!=0);

}

var table_remote_upload_turnoff = 0;
function Show_Remote_Transmission_downlist()
{
    if (table_remote_upload_turnoff == 0)
    {
        table_remote_upload_turnoff = 1;
        document.getElementById("table_remote_upload").style.display = "";
    }
    else
    {
        table_remote_upload_turnoff = 0;
        document.getElementById("table_remote_upload").style.display = "none";
    }
    
}

function Apply_Remote_Transmission_downlist()
{
    //是否启用
    var remote_group_is1 = document.getElementsByName("remote_group_is")[0].checked;
    var remote_group_is2 = document.getElementsByName("remote_group_is")[1].checked;
    //以太网工作模式 
    var remote_working_type = document.getElementById("remote_working_type").value;
    var remote_working_cs = document.getElementById("remote_working_cs").value;
    //协议
    var remote_proto = document.getElementById("remote_proto").value;
    var remote_proto_ms = document.getElementById("remote_proto_ms").value;

    //设备NM号（0-20位）
    //var remote_dev = document.getElementById("remote_dev").value;
    //域名地址
    var remote_ip = document.getElementById("remote_ip").value;
    //域名端口
    var remote_port = document.getElementById("remote_port").value;
    //上传间隔
    var remote_upload_interval = document.getElementById("remote_upload_interval").value;
    //上传的数据（注意“remote_parameter”值）
    /*var remote_parameter = "";
    for (var i = 0; i < Remote_Parameter_Items; i++)
    {
        if (document.getElementById("remote_upload_data" + i).checked)
        {
            remote_parameter += "," + i;
        }
    }
    remote_parameter = remote_parameter.substring(1);*/

    //是否启用心跳包
    var remote_heartbeat1 = document.getElementsByName("remote_heartbeat")[0].checked;
    var remote_heartbeat2 = document.getElementsByName("remote_heartbeat")[1].checked;

    if (chk2(remote_group_is1, remote_group_is2, "是否启用")) return;
    if( remote_group_is1 ) {
        if (chk2(remote_working_type, remote_working_cs, "以太网工作模式")) return;
        if (chk2(remote_proto, remote_proto_ms, "协议")) return;
        //if (chk(remote_dev, "设备NM号")) return;
        if( Number(remote_working_cs) != 1 ) {
            if (chk(remote_ip, "域名地址")) return;
        }
        if (chk(remote_port, "域名端口")) return;
        if (chk(remote_upload_interval, "上传间隔")) return;
        //if (chk(remote_parameter, "上传的数据")) return;

        if (chk2(remote_heartbeat1, remote_heartbeat2, "是否启用心跳包")) return;
    }

    //注：在此处填写保存代码即可；
    var n = document.getElementById("remote_group").selectedIndex;
    var setval = {
        n:Number(n), 
        en:remote_group_is1?1:0, 
        tt:Number(remote_working_type), 
        pt:Number(remote_proto), 
        cs:Number(remote_working_cs), 
        ms:Number(remote_proto_ms), 
        pe:remote_ip, 
        po:Number(remote_port), 
        it:Number(remote_upload_interval), 
        kl:remote_heartbeat1?1:0
    };
    
    MyGetJSON("正在设置以太网 TCP/IP配置","/cgi-bin/setTcpipCfg?","arg="+JSON.stringify(setval));
}


//本地网络配置
function Show_Local_Network_Configuration()
{
    //*注：在此处编写获取代码；然而调用如下函数，实现页面显示；
    //Page_Local_Network_Configuration("注：此处传入一个对象");
    MyGetJSON("","/cgi-bin/getNetCfg?","", function (info)
    {
        Page_Local_Network_Configuration(info)
    });
}

function Page_Local_Network_Configuration(cfg)
{
    //DHCP开关
    setValue('local_dhcp', cfg.dh);
    //IP 地址
    setValue('local_ip', cfg.ip);
    //子网掩码
    setValue('local_mask', cfg.mask);
    //网关
    setValue('local_gateway', cfg.gw);
    //DNS
    setValue('local_dns', cfg.dns);
    
    Show_Local_Setup();
}

function Apply_Local_Network_Configuration()
{
    //IP 地址
    var local_dhcp = document.getElementById("local_dhcp").value;
    //IP 地址
    var local_ip = document.getElementById("local_ip").value;
    //子网掩码
    var local_mask = document.getElementById("local_mask").value;
    //网关
    var local_gateway = document.getElementById("local_gateway").value;
    //DNS
    var local_dns = document.getElementById("local_dns").value;

    if (chk(local_dhcp, "DHCP开关")) return;
    if( Number(local_dhcp) != 1 ) {
        if (chk(local_ip, "IP 地址")) return;
        if (chk(local_mask, "子网掩码")) return;
        if (chk(local_gateway, "网关")) return;
        if (chk(local_dns, "DNS")) return;
    }
    
    var setval = {
        dh:Number(local_dhcp), 
        ip:local_ip, 
        mask:local_mask, 
        gw:local_gateway, 
        dns:local_dns
    };
    //注：在此处填写保存代码即可；
    MyGetJSON("正在配置网络,请稍后...","/cgi-bin/setNetCfg?","arg=" + JSON.stringify(setval));
}


//ZIGBEE 配置
function Show_ZIGBEE()
{
    //*注：在此处编写获取代码；然而调用如下函数，实现页面显示；
    Page_ZIGBEE("注：此处传入一个对象");
}

function Page_ZIGBEE(obj)
{
    //ZIGBEE工作模式
    var ZIGBEE_Working_Mode = document.getElementById("ZIGBEE_Working_Mode");
    for (var i = 0; i < ZIGBEE_Working_Mode.options.length; i++)
    {
        if (ZIGBEE_Working_Mode.options[i].value == "此处填写对象.值")
        {
            ZIGBEE_Working_Mode.selectedIndex = i;
            break;
        }
    }
    //panID
    setValue('ZIGBEE_panID', obj.null);
    //本地网络地址
    setValue('ZIGBEE_Local_Address', obj.null);
    //本地MAC地址
    setValue('ZIGBEE_Local_Mac', obj.null);
    //目的网络地址
    setValue('ZIGBEE_Destination_Address', obj.null);
    //目的MAC地址
    setValue('ZIGBEE_Destination_Mac', obj.null);
    //信道
    setValue('ZIGBEE_Channel', obj.null);
    //发送模式
    var ZIGBEE_Transmission_Mode = document.getElementById("ZIGBEE_Transmission_Mode");
    for (var i = 0; i < ZIGBEE_Transmission_Mode.options.length; i++)
    {
        if (ZIGBEE_Transmission_Mode.options[i].value == "此处填写对象.值")
        {
            ZIGBEE_Transmission_Mode.selectedIndex = i;
            break;
        }
    }

}


function Apply_ZIGBEE()
{
    //ZIGBEE工作模式
    var v = document.getElementById("ZIGBEE_Working_Mode").selectedIndex;
    var ZIGBEE_Working_Mode_ = document.getElementById("ZIGBEE_Working_Mode").options[v].text;

    //panID
    var ZIGBEE_panID = document.getElementById("ZIGBEE_panID").value;
    //本地网络地址
    var ZIGBEE_Local_Address = document.getElementById("ZIGBEE_Local_Address").value;
    //本地MAC地址
    var ZIGBEE_Local_Mac = document.getElementById("ZIGBEE_Local_Mac").value;
    //目的网络地址
    var ZIGBEE_Destination_Address = document.getElementById("ZIGBEE_Destination_Address").value;
    //目的MAC地址
    var ZIGBEE_Destination_Mac = document.getElementById("ZIGBEE_Destination_Mac").value;
    //信道
    var ZIGBEE_Channel = document.getElementById("ZIGBEE_Channel").value;
    //发送模式
    v = document.getElementById("ZIGBEE_Transmission_Mode").selectedIndex;
    var ZIGBEE_Transmission_Mode_ = document.getElementById("ZIGBEE_Transmission_Mode").options[v].text;
    
    if (chk(ZIGBEE_Working_Mode_, "ZIGBEE工作模式")) return;
    if (chk(ZIGBEE_panID, "panID")) return;
    if (chk(ZIGBEE_Local_Address, "本地网络地址")) return;
    if (chk(ZIGBEE_Destination_Address, "目的网络地址")) return;
    if (chk(ZIGBEE_Destination_Mac, "目的MAC地址")) return;
    if (chk(ZIGBEE_Channel, "信道")) return;
    if (chk(ZIGBEE_Transmission_Mode_, "发送模式")) return;


    //注：在此处填写保存代码即可；
    msg();
}




//LORA配置
function Show_LORA()
{
    //*注：在此处编写获取代码；然而调用如下函数，实现页面显示；
    Page_LORA("注：此处传入一个对象");
}

function Page_LORA(obj)
{
    //载波频率
    setValue('LORA_Carrier_Frequency', obj.null);
    //扩频因子
    setValue('LORA_Spreading_Factor', obj.null);
    //工作模式
    var LORA_Working_Mode = document.getElementById("LORA_Working_Mode");
    for (var i = 0; i < LORA_Working_Mode.options.length; i++)
    {
        if (LORA_Working_Mode.options[i].value == "此处填写对象.值")
        {
            LORA_Working_Mode.selectedIndex = i;
            break;
        }
    }
    //扩频带宽
    setValue('LORA_Spread', obj.null);
    //用户ID
    setValue('LORA_USERID', obj.null);
    //网络ID
    setValue('LORA_NETWORKID', obj.null);
    //发射功率
    var LORA_Transmit_Power = document.getElementById("LORA_Transmit_Power");
    for (var i = 0; i < LORA_Transmit_Power.options.length; i++)
    {
        if (LORA_Transmit_Power.options[i].value == "此处填写对象.值")
        {
            LORA_Transmit_Power.selectedIndex = i;
            break;
        }
    }

}


function Apply_LORA()
{
    //载波频率
    var LORA_Carrier_Frequency = document.getElementById("LORA_Carrier_Frequency").value;
    //扩频因子
    var LORA_Spreading_Factor = document.getElementById("LORA_Spreading_Factor").value;
    //工作模式
    var v = document.getElementById("LORA_Working_Mode").selectedIndex;
    var LORA_Working_Mode_ = document.getElementById("LORA_Working_Mode").options[v].text;
    
    //扩频带宽
    var LORA_Spread = document.getElementById("LORA_Spread").value;
    //用户ID
    var LORA_USERID = document.getElementById("LORA_USERID").value;
    //网络ID
    var LORA_NETWORKID = document.getElementById("LORA_NETWORKID").value;
    //发射功率
    v = document.getElementById("LORA_Transmit_Power").selectedIndex;
    var LORA_Transmit_Power_ = document.getElementById("LORA_Transmit_Power").options[v].text;
    
    if (chk(LORA_Carrier_Frequency, "载波频率")) return;
    if (chk(LORA_Spreading_Factor, "扩频因子")) return;
    if (chk(LORA_Working_Mode_, "工作模式")) return;
    if (chk(LORA_Spread, "扩频带宽")) return;
    if (chk(LORA_USERID, "用户ID")) return;
    if (chk(LORA_NETWORKID, "网络ID")) return;
    if (chk(LORA_Transmit_Power_, "发射功率")) return;


    //注：在此处填写保存代码即可；
    msg();
}




//GPRS 配置——工作模式及中心配置
function Show_GPRS_Transmission()
{
    //每次选择时，都会调用该函数；
    //获取【GPRS 配置——工作模式及中心配置】；=====================================================================>需要调用【设备接口】获取
    //如下是例子，例如：
    //Tabel_GPRS_Parameter(new Array(new Array("AAA", "BBB", "CCC", "DDD", "EEE", "FFF", "GGG", "HHH", "JJJ"), new Array("KKK", "LLL", "MMM", "OOO", "PPP", "QQQ", "RRR", "SSS", "TTT"), new Array("UUU", "VVV", "WWW")));
    //-----------------------------------------------------------------------------------------------------------------------

    var val = document.getElementById("GPRS_group").value;
    var setval = { n:(Number(val)+4) };
    
    MyGetJSON("正在获取GRPS TCP/IP配置","/cgi-bin/getTcpipCfg?","arg="+JSON.stringify(setval), function (info)
    {
        Page_GPRS_Transmission(info)
    });
}


var GPRS_Parameter_Items = 0;
function Tabel_GPRS_Parameter(obj)
{
    var str = "<table id='table_gprs_upload' style='display:none'>";

    try
    {
        var k = 0;
        for (var n = 0; n < obj.length; n++)
        {
            str += "<tr>";
            for (var j = 0; j < obj[n].length; j++)
            {
                str += "<td><input name='GPRS_upload_data' type='checkbox' id='GPRS_upload_data" + k + "' value='" + k + "' />&nbsp;参数" + obj[n][j] + "</td>";
                k++;
            }
            str += "</tr>";
        }
        GPRS_Parameter_Items = k;
    }
    catch (e)
    {
    }

    str += "</table>";

    document.getElementById("div_gprs_parameter").innerHTML = str;

}


function Page_GPRS_Transmission(cfg)
{
    //是否启用 
    document.getElementsByName("GPRS_group_is")[0].checked = (Number(cfg.en)!=0);
    document.getElementsByName("GPRS_group_is")[1].checked = !(Number(cfg.en)!=0);
    //以太网工作模式
    setValue('GPRS_working_type', cfg.tt);
    setValue('GPRS_working_cs', cfg.cs);
    //协议
    setValue('GPRS_proto', cfg.pt);
    setValue('GPRS_proto_ms', cfg.ms);
    //设备NM号（0-20位）
    //setValue('remote_dev', obj.null);
    //域名地址
    setValue('GPRS_peer_ip', cfg.pe);
    //域名端口
    setValue('GPRS_peer_port', cfg.po);
    //上传间隔
    setValue('GPRS_upload_interval', cfg.it);

    //上传的数据; =====================================================================>千万注意此处，绑定方式！！
    /*for (var i = 0; i < Remote_Parameter_Items; i++)
    {
        document.getElementById("GPRS_upload_data" + i).checked = true;
    }*/

    //是否启用心跳包 
    document.getElementsByName("GPRS_heartbeat")[0].checked = (Number(cfg.kl)!=0);
    document.getElementsByName("GPRS_heartbeat")[1].checked = !(Number(cfg.kl)!=0);

}


var table_gprs_upload_turnoff = 0;
function Show_gprs_Transmission_downlist()
{
    if (table_gprs_upload_turnoff == 0)
    {
        table_gprs_upload_turnoff = 1;
        document.getElementById("table_gprs_upload").style.display = "";
    }
    else
    {
        table_gprs_upload_turnoff = 0;
        document.getElementById("table_gprs_upload").style.display = "none";
    }

}


function Apply_gprs_Transmission_downlist()
{
    //是否启用
    var GPRS_group_is1 = document.getElementsByName("GPRS_group_is")[0].checked;
    var GPRS_group_is2 = document.getElementsByName("GPRS_group_is")[1].checked;
    //以太网工作模式 
    var GPRS_working_type = document.getElementById("GPRS_working_type").value;
    var GPRS_working_cs = document.getElementById("GPRS_working_cs").value;
    //协议
    var GPRS_proto = document.getElementById("GPRS_proto").value;
    var GPRS_proto_ms = document.getElementById("GPRS_proto_ms").value;

    //设备NM号（0-20位）
    //var GPRS_dev_ = document.getElementById("GPRS_dev").value;
    //域名地址
    var GPRS_peer_ip = document.getElementById("GPRS_peer_ip").value;
    //域名端口
    var GPRS_peer_port = document.getElementById("GPRS_peer_port").value;
    //上传间隔
    var GPRS_upload_interval_ = document.getElementById("GPRS_upload_interval").value;

    //上传的数据（注意“remote_parameter”值）
    /*var GPRS_parameter = "";
    for (var i = 0; i < GPRS_Parameter_Items; i++)
    {
        if (document.getElementById("GPRS_upload_data" + i).checked)
        {
            GPRS_parameter += "," + i;
        }
    }
    GPRS_parameter = GPRS_parameter.substring(1);*/

    //是否启用心跳包
    var GPRS_heartbeat1 = document.getElementsByName("GPRS_heartbeat")[0].checked;
    var GPRS_heartbeat2 = document.getElementsByName("GPRS_heartbeat")[1].checked;

    if (chk2(GPRS_group_is1, GPRS_group_is2, "是否启用")) return;
    if( GPRS_group_is1 ) {
        if (chk2(GPRS_working_type, GPRS_working_cs, "以太网工作模式")) return;
        if (chk2(GPRS_proto, GPRS_proto_ms, "协议")) return;
        //if (chk(GPRS_dev_, "设备NM号")) return;
        if( Number(GPRS_working_cs) != 1 ) {
            if (chk(GPRS_peer_ip, "域名地址")) return;
        }
        if (chk(GPRS_peer_port, "域名端口")) return;
        if (chk(GPRS_upload_interval_, "上传间隔")) return;
        //if (chk(GPRS_parameter, "上传的数据")) return;
        if (chk2(GPRS_heartbeat1, GPRS_heartbeat2, "是否启用心跳包")) return;
    }

    //注：在此处填写保存代码即可；
    var val = getNumber("GPRS_group") + 4;
    var setval = {
        n:Number(val), 
        en:GPRS_group_is1?1:0, 
        tt:Number(GPRS_working_type), 
        pt:Number(GPRS_proto), 
        cs:Number(GPRS_working_cs), 
        ms:Number(GPRS_proto_ms), 
        pe:GPRS_peer_ip, 
        po:Number(GPRS_peer_port), 
        it:Number(GPRS_upload_interval_), 
        kl:GPRS_heartbeat1?1:0
    };
    MyGetJSON("正在设置GPRS TCP/IP配置","/cgi-bin/setTcpipCfg?","arg="+JSON.stringify(setval));
}



//GPRS 工作参数配置
function Show_GPRS_Config()
{
    //*注：在此处编写获取代码；然而调用如下函数，实现页面显示；
    
    Page_GPRS_Config("注：此处传入一个对象");
}

function Page_GPRS_Config(obj)
{
    //工作模式
    var GPRS_Config_Mode = document.getElementById("GPRS_Config_Mode");
    for (var i = 0; i < GPRS_Config_Mode.options.length; i++)
    {
        if (GPRS_Config_Mode.options[i].value == "此处填写对象.值")
        {
            GPRS_Config_Mode.selectedIndex = i;
            break;
        }
    }
    
    //激活方式
    var GPRS_Config_Activation = document.getElementById("GPRS_Config_Activation");
    for (var i = 0; i < GPRS_Config_Activation.options.length; i++)
    {
        if (GPRS_Config_Activation.options[i].value == "此处填写对象.值")
        {
            GPRS_Config_Activation.selectedIndex = i;
            break;
        }
    }
    
    //调试等级
    var GPRS_Config_Level = document.getElementById("GPRS_Config_Level");
    for (var i = 0; i < GPRS_Config_Level.options.length; i++)
    {
        if (GPRS_Config_Level.options[i].value == "此处填写对象.值")
        {
            GPRS_Config_Level.selectedIndex = i;
            break;
        }
    }
    

    //SIM卡号码
    setValue('GPRS_Config_SIM', obj.null);
    //数据帧时间
    setValue('GPRS_Config_MS', obj.null);
    //自定义注册包
    setValue('GPRS_Config_Reg', obj.null);
    //自定义心跳包
    setValue('GPRS_Config_Jump', obj.null);
    //重连次数
    setValue('GPRS_Config_Time', obj.null);

}


function Apply_GPRS_Config()
{
    //工作模式
    var v = document.getElementById("GPRS_Config_Mode").selectedIndex;
    var GPRS_Config_Mode_ = document.getElementById("GPRS_Config_Mode").options[v].text;
    
    //激活方式
    v = document.getElementById("GPRS_Config_Activation").selectedIndex;
    var GPRS_Config_Activation_ = document.getElementById("GPRS_Config_Activation").options[v].text;
    
    //调试等级
    v = document.getElementById("GPRS_Config_Level").selectedIndex;
    var GPRS_Config_Level_ = document.getElementById("GPRS_Config_Level").options[v].text;
    

    //SIM卡号码
    var GPRS_Config_SIM = document.getElementById("GPRS_Config_SIM").value;
    //数据帧时间
    var GPRS_Config_MS = document.getElementById("GPRS_Config_MS").value;
    //自定义注册包
    var GPRS_Config_Reg = document.getElementById("GPRS_Config_Reg").value;
    //自定义心跳包
    var GPRS_Config_Jump = document.getElementById("GPRS_Config_Jump").value;
    //重连次数
    var GPRS_Config_Time = document.getElementById("GPRS_Config_Time").value;


    if (chk(GPRS_Config_Mode_, "工作模式")) return;
    if (chk(GPRS_Config_Activation_, "激活方式")) return;
    if (chk(GPRS_Config_Level_, "调试等级")) return;
    if (chk(GPRS_Config_SIM, "SIM卡号码")) return;
    if (chk(GPRS_Config_MS, "数据帧时间")) return;
    if (chk(GPRS_Config_Reg, "自定义注册包")) return;
    if (chk(GPRS_Config_Jump, "自定义心跳包")) return;
    if (chk(GPRS_Config_Time, "重连次数")) return;


    //注：在此处填写保存代码即可；
    msg();
}


//无线配置
function Show_GPRS_Wireless()
{
    //*注：在此处编写获取代码；然而调用如下函数，实现页面显示；
    //Page_GPRS_Wireless("注：此处传入一个对象");
    
    MyGetJSON("正在获取GPRS配置","/cgi-bin/getGPRSCfg?","", function (info)
    {
        Page_GPRS_Wireless(info)
    });
}

function Page_GPRS_Wireless(info)
{
    //无线网络APN
    setValue('GPRS_Wireless_APN', info.apn);
    //APN用户名
    setValue('GPRS_Wireless_User', info.user);
    //APN密码
    setValue('GPRS_Wireless_PWD', info.psk);
    //APN拨号号码
    setValue('GPRS_Wireless_NUM', info.num);
    //短信中心号码
    setValue('GPRS_Wireless_Center', info.cent);
}


function Apply_GPRS_Wireless()
{
    //无线网络APN
    var GPRS_Wireless_APN = document.getElementById("GPRS_Wireless_APN").value;
    //APN用户名
    var GPRS_Wireless_User = document.getElementById("GPRS_Wireless_User").value;
    //APN密码
    var GPRS_Wireless_PWD = document.getElementById("GPRS_Wireless_PWD").value;
    //APN拨号号码
    var GPRS_Wireless_NUM = document.getElementById("GPRS_Wireless_NUM").value;
    //短信中心号码
    var GPRS_Wireless_Center = document.getElementById("GPRS_Wireless_Center").value;

    if (chk(GPRS_Wireless_APN, "无线网络APN")) return;
    if (chk(GPRS_Wireless_User, "APN用户名")) return;
    if (chk(GPRS_Wireless_PWD, "APN密码")) return;
    if (chk(GPRS_Wireless_NUM, "APN拨号号码")) return;
    if (chk(GPRS_Wireless_Center, "短信中心号码")) return;

    //注：在此处填写保存代码即可；
    var setval = {
        apn:GPRS_Wireless_APN, 
        user:GPRS_Wireless_User, 
        psk:GPRS_Wireless_PWD, 
        num:GPRS_Wireless_NUM, 
        cent:GPRS_Wireless_Center
    };
    
    MyGetJSON("正在设置GPRS配置信息","/cgi-bin/setGPRSCfg?","arg=" + JSON.stringify(setval));
}


//串口配置
function Show_Serial_Port()
{
    var n = document.getElementById("Serial_Port_Select").selectedIndex;
    //var Serial_Port_Select = document.getElementById("Serial_Port_Select").options[v].text;
    //alert('当前选择【' + Serial_Port_Select + '】请此处添加代码。');
    //【你需要添加代码】通过“Serial_Port_Select”序号查找到对应的信息；
    //再把“对应信息”传给Page_Remote_Transmission函数；
    //Page_Serial_Port(Serial_Port_Select);
        
    var setval = {
        n:Number(n)
    };
    
    MyGetJSON("正在获取串口配置信息","/cgi-bin/getUartCfg?","arg=" + JSON.stringify(setval), function( cfg ){
        Page_Serial_Port(cfg)
    });
}


function Page_Serial_Port(cfg)
{
    //串口模式
    setValue("Serial_Port_Mode", cfg.ut);

    //协议配置
    setValue("Serial_Port_proto", cfg.po);
    setValue("Serial_Port_proto_ms", cfg.ms);
    //mdobus从机地址
    setValue("Serial_Port_ModbusAddr", cfg.ad);
    
    //串口波特率
    setValue("Serial_Port_Rate", cfg.bd);
    //串口校验位
    //setValue("Serial_Port_parity", cfg.py);

    //采集间隔（MS）
    setValue("Serial_Port_Interval", cfg.in);

    //数据位+停止位
    setValue("Serial_Port_Data", Number(cfg.da * 10) + cfg.st );
    
}

function Apply_Serial_Port()
{
    //选择串口
    var Serial_Port_Select = document.getElementById("Serial_Port_Select").value;
    
    var Serial_Port_Mode = document.getElementById("Serial_Port_Mode").value;
    
    //协议配置
    var Serial_Port_proto = document.getElementById("Serial_Port_proto").value;
    var Serial_Port_proto_ms = document.getElementById("Serial_Port_proto_ms").value;
    //mdobus从机地址
    var Serial_Port_ModbusAddr = document.getElementById("Serial_Port_ModbusAddr").value;
    
    //串口波特率
    var Serial_Port_Rate = document.getElementById("Serial_Port_Rate").value;
    //var Serial_Port_parity = document.getElementById("Serial_Port_parity").value;

    //采集间隔（MS）
    var Serial_Port_Interval = document.getElementById("Serial_Port_Interval").value;
    
    //数据位
    var Serial_Port_data_stop = document.getElementById("Serial_Port_Data").value;

    if (chk(Serial_Port_Select, "选择串口")) return;
    if (chk(Serial_Port_Mode, "选择串口模式")) return;
    if (chk2(Serial_Port_proto, Serial_Port_proto_ms, "协议配置")) return;
    if (chk(Serial_Port_Rate, "串口波特率")) return;
    if( Number(Serial_Port_proto_ms) ==  0 ) {
        if (chk(Serial_Port_ModbusAddr, "Modbus从机地址")) return;
    }
    //if (chk(Serial_Port_parity, "校验位")) return;
    if (chk(Serial_Port_Interval, "采集间隔")) return;
    
    Serial_Port_data_stop = Number(Serial_Port_data_stop);
    var Serial_Port_data = parseInt(Serial_Port_data_stop / 10);
    var Serial_Port_stop = parseInt(Serial_Port_data_stop) % 10;

    //注：在此处填写保存代码即可；
    var setval = {
        n:Number(Serial_Port_Select), 
        bd:Number(Serial_Port_Rate), 
        ut:Number(Serial_Port_Mode), 
        po:Number(Serial_Port_proto), 
        py:0, 
        ms:Number(Serial_Port_proto_ms), 
        ad:Number(Serial_Port_ModbusAddr), 
        in:Number(Serial_Port_Interval), 
        da:Number(Serial_Port_data), 
        st:Number(Serial_Port_stop)
    };
    
    MyGetJSON("正在设置串口配置信息","/cgi-bin/setUartCfg?","arg=" + JSON.stringify(setval));
}









//开关量输入设置
function Show_Switch_Cfg()
{
    //*注：在此处编写获取代码；然而调用如下函数，实现页面显示；
    //Page_Switch("注：此处传入一个对象");
    
    MyGetJSON("正在获取输入模块配置","/cgi-bin/getDiCfg?","", function( info ){
        Page_Switch_Cfg(info.cfg);
    });
}

function Page_Switch_Cfg(cfg)
{
    for(var i = 0; i < 8; i++) {
        document.getElementsByName("road_turnno")[i].checked = Number(cfg[i].en!=0);
        setValue("switch_interval"+(i+1), cfg[i].in);
        setValue("switch_expression"+(i+1), cfg[i].exp);
    }
}

function Show_Switch_Value()
{
    MyGetJSON("","/cgi-bin/getDiValue?","", function( info ){
        Page_Switch_Value(info.di);
    });
}

function Page_Switch_Value(di)
{
    for(var i = 0; i < 4; i++) {
        document.getElementById("switch_state"+(i+1)).innerHTML = Number(di[i].va!=0) ? "<font color='Green'>【高】</font>" : "<font color='Green'>【低】</font>";
    }    
    for(var i = 4; i < 8; i++) {
        document.getElementById("switch_state"+(i+1)).innerHTML = Number(di[i].va!=0) ? "<font color='Green'>【打开】</font>" : "<font color='Green'>【关闭】</font>";
    }
}

function Apply_Switch()
{
    var inputset = {
        cfg:new Array()
    };
    
    for(var i = 0; i < 8; i++) {
        inputset.cfg[i] = { 
            n:i, 
            en:(document.getElementsByName("road_turnno")[i].checked?1:0), 
            in:getNumber("switch_interval"+(i+1)), 
            exp:getValue("switch_expression"+(i+1))
        };
    }

    //注：在此处填写保存代码即可；
    MyGetJSON("正在设置输入模块配置","/cgi-bin/setDiCfg?","arg=" + JSON.stringify(inputset));
}


//开关量输出设置
function Show_Output_Cfg()
{
    //*注：在此处编写获取代码；然而调用如下函数，实现页面显示；
    //Page_Output("注：此处传入一个对象");
    MyGetJSON("正在获取输出模块配置","/cgi-bin/getDoCfg?","",function( info ){
        Page_Output_Cfg(info.cfg);
    });
}

function Page_Output_Cfg(cfg)
{    
    for(var i = 0; i < 6; i++) {
        setValue( "Output_Expression"+(i+1), cfg[i].exp );
    }
}

function Show_Output_Value()
{
    //*注：在此处编写获取代码；然而调用如下函数，实现页面显示；
    //Page_Output("注：此处传入一个对象");
    MyGetJSON("正在获取输出模块信息","/cgi-bin/getDoValue?","",function( info ){
        Page_Output_Value(info.do);
    });
}

function Page_Output_Value(_do)
{
    for(var i = 0; i < 4; i++) {
        setValue("Output_State"+(i+1), 0==Number(_do[i].va)?"开":"关");
    }
    
    for(var i = 4; i < 6; i++) {
        setValue("Output_State"+(i+1), 0==Number(_do[i].va)?"低":"高");
    }
}

function OutPutClick(n)
{    
    var OutputSet = {
        do:new Array()
    };
    n=n-1;
    if( n < 4 ) {
        OutputSet.do[n] = { n:n, va:(getValue("Output_State"+(n+1))=="开"?1:0) };
    } else {
        OutputSet.do[n] = { n:n, va:(getValue("Output_State"+(n+1))=="高"?0:1) };
    }
        
    MyGetJSON("正在设置输出模块信息","/cgi-bin/setDoValue?","arg=" + JSON.stringify(OutputSet),function( info ){
        Show_Output_Value();
    });
}

function Apply_Output()
{

    var OutputCfg = {
        cfg:new Array()
    };
    for(var i = 0; i < 6; i++) {
        OutputCfg.cfg[i] = { 
            n:i, 
            exp:(getValue("Output_Expression"+(i+1)))
        };
    }

    //注：在此处填写保存代码即可；
    MyGetJSON("正在设置输出模块信息","/cgi-bin/setDoCfg?","arg=" + JSON.stringify(OutputCfg));
}


//模拟量输入设置
function Show_Input()
{
    var n = document.getElementById("Input_Select").value;
    //var Input_Select = document.getElementById("Input_Select").options[v].text;
    //alert('当前选择【' + Input_Select + '】请此处添加代码。');
    //========================================================================>
    var setval = {
        n:Number(n)
    };
    
    MyGetJSON("正在获取模拟量输入配置","/cgi-bin/getAnalogCfg?", "arg=" + JSON.stringify(setval),function( cfg ){
        Page_Input(cfg);
    });
}

function Show_Input_Range_Electrical(cfg)
{
    //电参数实测值    
    var n = document.getElementById("Input_Select").value;
    //var Input_Select = document.getElementById("Input_Select").options[v].text;
    //alert('当前选择【' + Input_Select + '】请此处添加代码。');
    //========================================================================>
    var setval = {
        n:Number(n)
    };
    
    MyGetJSON("","/cgi-bin/getAnalogCfg?", "arg=" + JSON.stringify(setval),function( info ){
        setValue('Input_Range_Engineering', info.eg);
        setValue('Input_Range_Electrical', info.va);
    });
}

function Page_Input(cfg)
{
    //是否启用
    document.getElementsByName("Input_Group_Is")[0].checked = (Number(cfg.en)!=0);
    document.getElementsByName("Input_Group_Is")[1].checked = !(Number(cfg.en)!=0);

    //采集间隔
    setValue('Input_Interval', cfg.it);

    //数据格式
    setValue("Input_Format", cfg.ut );
    
    //量程设置
    setValue("Input_Range", cfg.rg);

    //工程量实测值
    setValue('Input_Range_Engineering', cfg.eg);
    //电参数实测值
    setValue('Input_Range_Electrical', cfg.va);

    //最大量程
    setValue('Input_Range_Max', cfg.ea);
    //最小量程
    setValue('Input_Range_Min', cfg.ei);
    //修正系数
    setValue('Input_Correction_Factor', cfg.ec);
    //表达式
    //setValue('Input_Expression', obj.null);
    
    Hide_Input();
}

function Hide_Input()
{
    var n = Number(document.getElementById("Input_Format").value);
    if ( 3 == n )
    {
        document.getElementById("Input_Show_Max").style.display = "";
        document.getElementById("Input_Show_Min").style.display = "";
        document.getElementById("Input_Show_Factor").style.display = "";
    }
    else
    {
        document.getElementById("Input_Show_Max").style.display = "none";
        document.getElementById("Input_Show_Min").style.display = "none";
        document.getElementById("Input_Show_Factor").style.display = "none";
    }
}

function Apply_Input()
{
    var Input_Select = document.getElementById("Input_Select").value;
    //是否启用
    var Input_Group_Is1 = document.getElementsByName("Input_Group_Is")[0].checked;
    var Input_Group_Is2 = document.getElementsByName("Input_Group_Is")[1].checked;
    
    //采集间隔
    var Input_Interval = document.getElementById("Input_Interval").value;
    //数据格式
    var Input_Format = document.getElementById("Input_Format").value;
    //量程设置
    var Input_Range = document.getElementById("Input_Range").value;
    //最大量程
    var Input_Range_Max = document.getElementById("Input_Range_Max").value;
    //最小量程
    var Input_Range_Min = document.getElementById("Input_Range_Min").value;
    //修正系数
    var Input_Correction_Factor = document.getElementById("Input_Correction_Factor").value;
    //表达式
    //var Input_Expression = document.getElementById("Input_Expression").value;

    if (chk2(Input_Group_Is1, Input_Group_Is2, "是否启用")) return;
    if( Input_Group_Is1 ) {
        if (chk(Input_Interval, "采集间隔")) return;
        if (chk(Input_Format, "数据格式")) return;
        if (chk(Input_Range, "量程设置")) return;

        if ( 3 == Input_Format )
        {
            if (chk(Input_Range_Max, "最大量程")) return;
            if (chk(Input_Range_Min, "最小量程")) return;
            if (chk(Input_Correction_Factor, "修正系数")) return;
        }
    }

    //if (chk(Input_Expression, "表达式")) return;

    //注：在此处填写保存代码即可；
    var setval = {
        n:Number(Input_Select), 
        en:Input_Group_Is1?1:0, 
        it:Number(Input_Interval), 
        rg:Number(Input_Range), 
        ut:Number(Input_Format), 
        ei:Number(Input_Range_Min), 
        ea:Number(Input_Range_Max), 
        ec:Number(Input_Correction_Factor)
    };    
    
    
    MyGetJSON("正在设置模拟量输入配置","/cgi-bin/setAnalogCfg?","arg=" + JSON.stringify(setval));
}

//=======================提交文件==========================
function Upload_File()
{
    var content = "";
    var filename = document.getElementById("file_upload").value;

    var getActiveXObject = new ActiveXObject("Scripting.FileSystemObject");
    //获取文件
    var getFile = getActiveXObject.GetFile(fileName);
    ////获取内容
    var openFile = getActiveXObject.OpenTextFile(fileName, 1);
    //一直到读完；
    while (!openFile.AtEndOfStream)
    {
        content += openFile.Readline();
    }
    
    
    //========================Post=========================

    var xhr = createXMLHttpRequest();
    if (xhr)
    {
        Show("正在提交文件,请稍后...");
        xhr.onreadystatechange = function ()
        {
            if (xhr.readyState == 4)
            {
                if (xhr.status == 200)
                {
                    var res = xhr.responseText;
                    if (res && 0 == res.ret)
                    {
                        alert("提交成功。");
                    }
                    else
                    {
                        alert("提交失败,请重试！");
                    }
                }
                else
                {
                    alert("提交超时,请重试！");
                }
            }
        }

        xhr.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
        xhr.open("POST","/cgi-bin/update?arg={'path':'" + getFile.Name + "','filelen':'" + getFile.size + "','content':'" + content + "'}", true);
        xhr.send();
    }
    
}