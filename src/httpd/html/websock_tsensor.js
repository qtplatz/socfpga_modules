var ws = null

plot_data = [ {
    x: [0,0,0],
    y: [1,2,3],
    type:'scatter',
    mode:'markers', marker:{size:5}
}],

layout = {
    hovermode:'closest',
    title:'Click on Points'
};

var tsensor_data = {
    time: []
    , celsius: []
    , type: 'scatter'
    , mode:'markers', marker:{size:5}
};

var tsensor_trace = {
    x: tsensor_data.time
    , y: tsensor_data.celsius
    , type: 'scatter'
};

var tsensor_layout = {
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

    Plotly.newPlot( 'plot', plot_data, layout ); // draw inital axes

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
        console.log( obj );
        if ( Object.prototype.toString.call( obj ) === '[object Array]' ) {
            tsensor_data.time = obj.filter( function(datum){ return new Date( datum.tick.tp ); } )
            tsensor_data.celsius = obj.filter( function(datum){ return datum.act; } );
        } else {
            d = new Date( obj.tsensor.data.tick.tp );
            $("#timestamp").html( d );

            tsensor_data.time.push( d );
            tsensor_data.celsius.push( obj.tsensor.data.act );

            while ( tsensor_data.time.length > 3600 ) {
                tsensor_data.time.shift();
                tsensor_data.celsius.shift();
            }
        }
        if ( tsensor_data.time.length >= 2 )  {
            Plotly.newPlot( 'plot', [ tsensor_trace ], tsensor_layout );
        }
    };
});
