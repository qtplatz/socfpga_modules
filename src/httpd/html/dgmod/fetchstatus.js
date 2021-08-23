
function set_delay_width( _this, delay_width, fcn ) {
/*
    if ( protocol.replicates == 0 ) {
        $(_this).find( '.enable' ).bootstrapToggle( 'off' );
    } else {
        $(_this).find( '.enable' ).bootstrapToggle( 'on' );
	$(_this).find( ':input[id=REPLICATES]' ).each( function(){ this.value = protocol.replicates; } );
    }
*/
    /*
    $(_this).find( ':input[id^=PULSE\\.DELAY]' ).each( function( i ){
	this.value = parseFloat( protocol.pulses[ i ].delay * 1.0e6 ).toFixed(3);
    });

    $(_this).find( ':input[id^=PULSE\\.WIDTH]' ).each( function( i ){
	this.value = parseFloat( protocol.pulses[ i ].width * 1.0e6 ).toFixed(3); // s -> us
    });
    */
    console.log( delay_width );
    console.log( fcn );
}


function updateProtocols( status ) {

    console.log( status );
    $('div #interval').find( ':input' ).each( function(){ // T_0
	this.value = status.interval * 1.0e6; // s -> us
    });

    $(status.delay_width).each( function( index ){
	set_delay_width( $('div #p\\[' + index + '\\]')[0], this, index );
    });

}

function fetchStatus() {

    $.ajax( { type: "GET"
              , url: "/fpga/dgmod/ctl$status.json"
              , dataType: "json"
            }).done( function( response ) {
                updateProtocols( response.status );
            }).fail( function( response ) {
                var obj = JSON.parse( response.responseText );
                updateProtocols( obj.protocols );
            });
}
