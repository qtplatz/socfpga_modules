var ws = null

$( document ).ready( function() {
    var uri = "ws://" + window.location.host;
    $("#uri").val( uri );
    ws = new WebSocket( uri, 'dgmod' )
    ws.onopen = function( ev ) {
        console.log( "connected" );
    };
    ws.onclose = function( ev ) {
        console.log( "disconnected" );
    };
    ws.onmessage = function( ev ) {
        obj = JSON.parse( ev.data );
        $("#tick").html( obj.dgmod.timestamp );
    };
});
