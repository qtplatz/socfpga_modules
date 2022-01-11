var ws = null

function tsensor_message(msg) {
    messages.innerText += msg + "\n";
    messages.scrollTop = messages.scrollHeight - messages.clientHeight;
};

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
        data = obj.tsensor.data;
        tick = obj.tsensor.tick;
        console.log( data.act + ", " + data.set + ", " + data.enable + ", " + data.flag + ", " + ", " + data.id )
        tsensor_message( JSON.stringify( data ) );
        if ( tick !== undefined ) {
            var dt = Date( tick.tp );
            $("#timestamp").html( dt.toString() );
        }
        // console.log( ev.data );
        // $("#tick").html( obj.dgmod.timestamp );
    };
});
