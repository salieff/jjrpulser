var heap, temp, digi;
var reloadPeriod = 1000;
var running = false;

function div(val, by){
    return (val - val % by) / by;
}

function fillUptime(res) {
    var millis = res.uptime;

    var seconds = div(res.uptime, 1000);
    millis %= 1000;

    var minutes = div(seconds, 60);
    seconds %= 60;

    var hours = div(minutes, 60);
    minutes %= 60;

    var days = div(hours, 24);
    hours %= 24;

    document.getElementById("uptime_days").innerHTML = "Дней без перезагрузки: " + days;
    document.getElementById("uptime_hours").innerHTML = "Часов без перезагрузки: " + hours;
    document.getElementById("uptime_minutes").innerHTML = "Минут без перезагрузки: " + minutes;
    document.getElementById("uptime_seconds").innerHTML = "Секунд без перезагрузки: " + seconds;
    document.getElementById("uptime_millis").innerHTML = "Миллисекунд без перезагрузки: " + millis;
}

function loadValues() {
    if (!running) return;
    var xh = new XMLHttpRequest();
    xh.onreadystatechange = function() {
        if (xh.readyState == 4) {
            if (xh.status == 200) {
                var res = JSON.parse(xh.responseText);
                heap.add(res.heap);
                temp.add(res.analog);
                digi.add(res.gpio);

                fillUptime(res);

                if (running) setTimeout(loadValues, reloadPeriod);
            } else running = false;
        }
    };
    xh.open("GET", "/all", true);
    xh.send(null);
};

function run() {
    if (!running) {
        running = true;
        loadValues();
    }
}

function capitalizeFLetter(string) {
    return string[0].toUpperCase() + string.slice(1);
}

function fillLEDModeBlock(container) {
    var color = container.dataset.color;
    var legend = document.createElement("legend");
    legend.innerHTML = (capitalizeFLetter(color) + " LED mode").fontcolor(color);

    container.appendChild(legend);

    ["Off", "Work", "Data", "Setup", "Error"].forEach(function(mode) {

        var radioButton = document.createElement("input");
        radioButton.type = "radio";
        radioButton.id = color + "ledmode" + mode.toLowerCase();
        radioButton.name = color + "ledmode";
        radioButton.value = mode.toLowerCase();

        var label = document.createElement("label");
        label.htmlFor = radioButton.id;
        label.innerHTML = mode;

        var radioDiv = document.createElement("div");
        radioDiv.className = "radiobtn";
        radioDiv.appendChild(radioButton);
        radioDiv.appendChild(label);

        container.appendChild(radioDiv);
    });
}

function onBodyLoad() {
    var refreshInput = document.getElementById("refresh-rate");
    refreshInput.value = reloadPeriod;
    refreshInput.onchange = function(e) {
        var value = parseInt(e.target.value);
        reloadPeriod = (value > 0) ? value : 0;
        e.target.value = reloadPeriod;
    }
    var stopButton = document.getElementById("stop-button");
    stopButton.onclick = function(e) {
        running = false;
    }
    var startButton = document.getElementById("start-button");
    startButton.onclick = function(e) {
        run();
    }

    // Example with 10K thermistor
    //function calcThermistor(v) {
    //  var t = Math.log(((10230000 / v) - 10000));
    //  t = (1/(0.001129148+(0.000234125*t)+(0.0000000876741*t*t*t)))-273.15;
    //  return (t>120)?0:Math.round(t*10)/10;
    //}
    //temp = createGraph(document.getElementById("analog"), "Temperature", 100, 128, 10, 40, false, "cyan", calcThermistor);

    temp = createGraph(document.getElementById("analog"), "Analog Input", 100, 128, 0, 1023, false, "cyan");
    heap = createGraph(document.getElementById("heap"), "Current Heap", 100, 125, 0, 30000, true, "orange");
    digi = createDigiGraph(document.getElementById("digital"), "GPIO", 100, 146, [0, 4, 5, 16], "gold");

    Array.from(document.getElementsByClassName("ledmode")).forEach(fillLEDModeBlock);

    run();
}
