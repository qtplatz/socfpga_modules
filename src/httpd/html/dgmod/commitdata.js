function findProtocol( json, protoId, elm )
{
    var protoData = {
	index : 0
	, replicates : 1
	, pulses : []
    };

    protoData[ "index" ] = protoId;
    protoData[ "replicates" ] = 1;

    console.log( "findProtocol: " + protoId );

    var enable;
    $(elm).find( ':input[id^=enable]' ).each( function( i ){
	enable = this.checked;
    });

    $(elm).find( ':input[id^=REPLICATES]' ).each( function( i ){
	protoData[ "replicates" ] = enable ? this.value : 0;
    });

    var pulse = { delay : 0.0, width : 0.0 };

    $(elm).find( ':input[id^=PULSE]' ).each( function( i ){
	if ( this.id.indexOf( "DELAY" ) >= 0 ) {
	    pulse.delay = this.value;
	} else {
	    pulse.width = this.value;
	    protoData.pulses.push( { "delay" : pulse.delay * 1.0e-6, "width" : pulse.width * 1.0e-6} ); // 'us' -> 's'
	}
    });

    // console.log( "prtoData: " + JSON.stringify( protoData ) );

    json.protocol.push( protoData );
}

function commitData()
{
    var json = {
	protocols : {
	    interval : 0.001
	    , protocol : []
	}
    };

    var interval = $( '#interval #interval' ).val();
    json.protocols.interval = interval * 1.0e-6; // 'us' -> 's'

    $("div [id^=p\\[]").each( function( index ){
	var protoId = $(this).attr( 'data-index' );
	findProtocol( json.protocols, protoId, this );
    });

    $.ajax( { type: "POST"
              , url: "/fpga/dgmod/ctl$commit"
              , dataType: "json"
              , contentType: 'application/json'
              , data: JSON.stringify( json ) } ).done( function( response ) {
                  console.log( response );
              } );
}
