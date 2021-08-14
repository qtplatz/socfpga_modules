
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
    $( '#connect' ).on( 'click', function( e ) {
        console.log( 'clicked' );
        ws = new WebSocket(uri.value);
        ws.onopen = function(ev) {
            showMessage("[connection opened]");
        };
        ws.onclose = function(ev) {
            showMessage("[connection closed]");
        };
        ws.onmessage = function(ev) {
            showMessage(ev.data);
        };
        ws.onerror = function(ev) {
            showMessage("[error]");
            console.log(ev);
        };
    });

    $( '#disconnect' ).on( 'click', function( e ) {
        console.log( 'disconnect clicked' );
        ws.close();
    });

    $( '#send' ).on( 'click', function ( e ) {
        ws.send(userName.value + ": " + sendMessage.value);
        sendMessage.value = "";
    });

    $( '#sendMessage' ).keyup( function( e ) {
        e.preventDefault();
        if (e.keyCode === 13) {
            send.click();
        }
    });
});
