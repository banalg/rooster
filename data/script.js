$(document).ready(function(){
    $("#appliquer").click(function(){
        var valeur = $("#choixDelayLed").val();
        $.post("delayLed",{
            valeurDelayLed: valeur
        });
    });
});

setInterval(function getData() {
    var xhttp = new XMLHttpRequest();

    xhttp.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == 200) {
            document.getElementById("valeurLuminosite").innerHTML = this.responseText;
        }
    };

    xhttp.open("GET", "lireLuminosite", true);
    xhttp.send();
}, 2000);

function closeDoor() {
    var xhttp = new XMLHttpRequest();
    xhttp.open("GET", "closeDoor", true);
    xhttp.send();
}

function openDoor() {
    var xhttp = new XMLHttpRequest();
    xhttp.open("GET", "openDoor", true);
    xhttp.send();
}