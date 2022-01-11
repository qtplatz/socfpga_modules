var ws = null

$( document ).ready( function() {
    var uri = "ws://" + window.location.host;
    $("#uri").val( uri );
    ws = new WebSocket( uri, 'tsensor' )
    ws.onopen = function( ev ) {
        console.log( "connected" );
        ws.send( JSON.stringify( { "tsensor": { "msg": "hello" }} ) );
    };

    ws.onclose = function( ev ) {
        console.log( "disconnected" );
    };

    ws.onmessage = function( ev ) {
        obj = JSON.parse( ev.data );
        console.log( ev.data );
        // $("#tick").html( obj.dgmod.timestamp );
    };
});
