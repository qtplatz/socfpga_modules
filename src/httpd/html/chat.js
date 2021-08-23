
var ws = null;

$( document ).ready( function() {
    var uri = "ws://" + window.location.host;
    $("#uri").val( uri );
});

function showMessage(msg) {
    messages.innerText += msg + "\n";
    messages.scrollTop = messages.scrollHeight - messages.clientHeight;
};

$(function() {
    $( '#websock-connect').change( function() {
        console.log( 'toggled' + $(this).prop('checked'));
        if ( $(this).prop('checked') == true ) {
            ws = new WebSocket(uri.value, 'chat')
            ws.onopen = function(ev) {
                showMessage("[connection opened]" + uri.value );
            };
            ws.onclose = function(ev) {
                showMessage("[connection closed]");
            };
            ws.onmessage = function(ev) {
                try {
                    obj = JSON.parse( ev.data );
                    if ( obj.chat !== undefined )
                        showMessage( obj.chat.user + " : " + obj.chat.msg );
                    if ( obj.tick !== undefined ) {
                        $("#timestamp").html( obj.tick.tp );
                    }
                } catch ( err ) {
                    console.log( err );
                }
                // showMessage(ev.data);
            };
            ws.onerror = function(ev) {
                showMessage("[error]" + ev );
                console.log(ev);
            };
        } else {
            ws.close();
        }
    });

    $( '#send' ).on( 'click', function ( e ) {
        ws.send( JSON.stringify( { "chat": { "user": userName.value, "msg": sendMessage.value }} ) );
        sendMessage.value = "";
    });

    $( '#sendMessage' ).keyup( function( e ) {
        e.preventDefault();
        if (e.keyCode === 13) {
            send.click();
        }
    });
});
