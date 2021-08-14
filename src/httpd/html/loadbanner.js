function loadBanner()
{
    $.ajax({ type: "GET", url: "/dg/ctl$banner", dataType: "html"
           }).done( function( response ) {
               $("#banner").html( response );
           }).fail( function( response ) {
               $("#banner").html( response.responseText );
           });
}

$(document).ready( function() {
    loadBanner();
})
