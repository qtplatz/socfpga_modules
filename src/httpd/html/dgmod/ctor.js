
var loadCounts = 0;

function loadCounter( proto ) {
    ++loadCounts;
    console.log( "protocol load count: " + loadCounts + " proto" + proto );
    if ( loadCounts == 4 ) {
        fetchStatus();
    }
}

function downloadOnload() {
    $("#p\\[0\\]").load( "protocol.html section", function(responseText,textStatus,XMLHttpRequest){
        var proto = 0;
        var t = $("#p\\[" + proto + "\\]")
        $(t).find( "#protocolIndex").text( "Protocol #" + (proto + 1) );
        $(t).find( ".enable" ).bootstrapToggle('disable');
        $(t).find( ".enable" ).attr( "id", "enable-" + proto );
        $(t).find( ".enable" ).attr( "proto", proto );
        loadCounter( proto );
    });
    $("#p\\[1\\]").load( "protocol.html section", function(responseText,textStatus,XMLHttpRequest){
        var proto = 1;
        var t = $("#p\\[" + proto + "\\]")
        $(t).find( "#protocolIndex").text( "Protocol #" + (proto + 1) );
        $(t).find( ".enable" ).bootstrapToggle();
        $(t).find( ".enable" ).attr( "id", "enable-" + proto );
        $(t).find( ".enable" ).attr( "proto", proto );
        loadCounter( proto );
    });
    $("#p\\[2\\]").load( "protocol.html section", function(responseText,textStatus,XMLHttpRequest){
        var proto = 2;
        var t = $("#p\\[" + proto + "\\]")
        $(t).find( "#protocolIndex").text( "Protocol #" + (proto + 1) );
        $(t).find( ".enable" ).bootstrapToggle();
        $(t).find( ".enable" ).attr( "id", "enable-" + proto );
        $(t).find( ".enable" ).attr( "proto", proto );
        loadCounter( proto );
    });
    $("#p\\[3\\]").load( "protocol.html section", function(responseText,textStatus,XMLHttpRequest){
        var proto = 3;
        var t = $("#p\\[" + proto + "\\]")
        $(t).find( "#protocolIndex").text( "Protocol #" + (proto + 1) );
        $(t).find( ".enable" ).bootstrapToggle();
        $(t).find( ".enable" ).attr( "id", "enable-" + proto );
        $(t).find( ".enable" ).attr( "proto", proto );
        loadCounter( proto );
    });
}

$(document).ready( function() {
    downloadOnload();
});
