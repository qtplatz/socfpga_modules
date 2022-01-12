var ws = null

var tsensor_data = {
    time: []
    , celsius: []
    , type: 'scatter'
    , mode:'markers', marker:{size:5}
};

var tsensor_trace = [ {
    x: tsensor_data.time
    , y: tsensor_data.celsius
    , type: 'scatter'
} ];

var tsensor_layout = {
    title: 'Temperature'
    , xaxis: {
        title: 'Elapsed time'
    }
    , yaxis: {
        title: 'Celsius'
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

    Plotly.newPlot( 'plot', tsensor_trace, tsensor_layout );
    // Plotly.newPlot( 'plot', plot_data, layout ); // draw inital axes

    var uri = "ws://" + window.location.host;
    $("#uri").val( uri );
    ws = new WebSocket( uri, 'tsensor' )

    ws.onopen = function( ev ) {
        ws.send( JSON.stringify( { "tsensor": { "msg": "hello" }} ) );
        ws.send( JSON.stringify( { "tsensor": { "msg": "fetch" }} ) );
    };

    ws.onclose = function( ev ) {
        console.log( "disconnected" );
    };

    ws.onmessage = function( ev ) {
        obj = JSON.parse( ev.data );
        //console.log( obj );
        if ( Object.prototype.toString.call( obj ) === '[object Array]' ) {
            tsensor_data.time.splice( 0, tsensor_data.time.length ); // equivalent to vector::clear() in c++
            tsensor_data.celsius.splice( 0, tsensor_data.celsius.length );
            for ( let i = 0; i < obj.length; ++i ) {
                tsensor_data.time.push( new Date( obj[i].tick.tp ) );
                tsensor_data.celsius.push( obj[i].act );
            }
            Plotly.newPlot( 'plot', tsensor_trace, tsensor_layout );
        } else {
            d = new Date( obj.tsensor.data.tick.tp );
            $("#timestamp").html( d );

            tsensor_data.time.push( d );
            tsensor_data.celsius.push( obj.tsensor.data.act );
            if ( tsensor_data.time.length > 1200 ) {
                tsensor_data.time.splice( 0, tsensor_data.time.length - 1100 );
                tsensor_data.celsius.splice( 0, tsensor_data.time.length - 1100 );
            }
        }
        if ( tsensor_data.time.length >= 2 )  {
            Plotly.redraw( 'plot' );
        }

        if ( obj.tsensor != undefined ) {
            if ( $('#master-switch').prop('checked') != obj.tsensor.data.enable ) {
                // $('#master-switch').bootstrapToggle( obj.tsensor.data.enable ? 'off' : 'on', true );
            }
            $('#server-master-state').html( "Peltier: " + obj.tsensor.data.enable);
            $('#server-control-flag').html( "Control: " + obj.tsensor.data.flag);
            $('#server-setpt').html(  "Head temp (set): " + obj.tsensor.data.set );
            $('#server-actual').html( "Head temp (act): " + obj.tsensor.data.act );
            terror = Math.abs( obj.tsensor.data.set - obj.tsensor.data.act );
            if ( terror > 5 ) {
                $('#server-values').css("background-color", "pink");
            } else {
                $('#server-values').css("background-color", "springgreen");
            }
        }
    };

});

$(function() {
    $('#master-switch').change(function() {
        ws.send( JSON.stringify( { "tsensor": { "enable": $(this).prop('checked') } } ) );
        $('#console-event').html('Toggle: ' + $(this).prop('checked'))
    })
})
