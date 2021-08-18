function loadBanner()
{
    $.ajax({ type: "GET", url: "/fpga/dgmod/revision", dataType: "html"
           }).done( function( response ) {
               $("#banner").html( response );
           }).fail( function( response ) {
               $("#banner").html( response.responseText );
           });
}

$(document).ready( function() {
    loadBanner();
})
