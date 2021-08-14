
function WebSocketTest() {

    if ("WebSocket" in window) {

        // alert( "WebSocket is supported by your Browser!" );

        // Let us open a web socket
        var url = "ws://" + window.location.host;
        console.log( "url: " + url );

        var ws = new WebSocket( url + "/echo" ); // "ws://mars:8080/echo");

        ws.onopen = function() {
            // Web Socket is connected, send data using send()
            ws.send("websock.js: Message to send");

            // alert("Message is sent...");
        };

        ws.onmessage = function (evt) {
            var received_msg = evt.data;
            console.log("Message is received..." + received_msg );
        };

        ws.onclose = function() {
            // websocket is closed.
            console.log("Connection is closed...");
        };

    } else {
        // The browser doesn't support WebSocket
        alert("WebSocket NOT supported by your Browser!");
    }
}
