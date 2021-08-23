/**
var source = new EventSource( '/dg/ctl$events' );

source.addEventListener( 'tick', function( e ) {
    var ev = document.getElementById( 'tick' );
    ev.innerHTML = e.data;
}, false );

source.onmessage = function(e) {
    var ev = document.getElementById( 'status' )
    if ( ev )
	ev.innerHTML = e.data
};

source.onerror = function( e ) {
    console.log( e )
};

source.onopen = function( e ) {
    var ev = document.getElementById('status')
    if ( ev )
	ev.innerHTML = "open";
}
*/

////////////////////

$(function() {
    $( "[id^=button-commit]" ).on( 'click', function( e ) {
	commitData();
    });

    $( "[id^=button-fetch]" ).on( 'click', function( e ) {
	fetchStatus();
    });

    $( "[id^=button-banner]" ).on( 'click', function( e ) {
        fetchBanner();
    });
});


////////////////////

function onChange( elm ) {

    var id = elm.id;
    var proto = parseInt( $(elm).attr( "proto" ) );
    //var data = { setpts: [ { id: elm.id, value: elm.checked, proto: $(elm).attr( "proto" ) } ] };

    if ( elm.checked ) {
        for ( i = 1; i < proto; ++i )
            $("#enable-" + i).bootstrapToggle( "on" );
    } else {
        for ( let i = proto + 1; i < 4; i++ )
            $("#enable-" + i).bootstrapToggle( "off" );
    }
}
