var ws = null

x = [100, 200, 300, 400, 500],
y = [10, 20, 30, 20, 10],

plot_data = [ {
    x:x,
    y:y,
    type:'scatter',
    mode:'markers', marker:{size:20}
}],

layout = {
    hovermode:'closest',
    title:'Click on Points'
};

var temp = {
    time: []
    , celsius: []
    , type: 'scatter'
};

var temp_trace = {
    x: temp.time
    , y: temp.celsius
    , type: 'scatter'
};

var temp_layout = {
    title: 'Temperature'
    , xaxis: {
        title: 'Elapsed time'
    }
    , yaxis: {
        title: 'degC'
    }
    , margin: { l: 80, r:50, b: 50, t: 0, pad: 0 }
    , height: 450
    , showlegend: true
};

function tsensor_message(msg) {
    //messages.innerText += msg + "\n";
    //messages.scrollTop = messages.scrollHeight - messages.clientHeight;
};

$( document ).ready( function() {

    // Plotly.newPlot( 'plot', plot_data, layout );
    var uri = "ws://" + window.location.host;
    $("#uri").val( uri );
    ws = new WebSocket( uri, 'tsensor' )
    ws.onopen = function( ev ) {
        ws.send( JSON.stringify( { "tsensor": { "msg": "hello" }} ) );
    };

    ws.onclose = function( ev ) {
        console.log( "disconnected" );
    };

    ws.onmessage = function( ev ) {
        obj = JSON.parse( ev.data );
        data = obj.tsensor.data;
        tick = obj.tsensor.tick;
        tsensor_message( JSON.stringify( data ) );

        d = new Date( tick.tp );
        $("#timestamp").html( d );

        temp.time.push( d );
        temp.celsius.push( data.act );
        console.log( temp_trace );
        if ( temp.time.length >= 2 )  {
            Plotly.newPlot( 'plot', [ temp_trace ], temp_layout );
        }
        console.log( temp );


    };
});
