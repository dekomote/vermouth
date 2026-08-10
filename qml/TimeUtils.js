.pragma library

// Format a play time given in seconds as "H:MM:SS". Returns an empty string
// when there is nothing to show (zero or invalid input).
function formatPlayTime(seconds) {
    if (!seconds || seconds <= 0)
        return "";
    seconds = Math.floor(seconds);
    var h = Math.floor(seconds / 3600);
    var m = Math.floor((seconds % 3600) / 60);
    var s = seconds % 60;
    function pad(n) {
        return n < 10 ? "0" + n : "" + n;
    }
    return h + ":" + pad(m) + ":" + pad(s);
}

// Parse a play time in "H:MM:SS", "M:SS" or plain-seconds form into seconds.
// Returns 0 for an empty string, or NaN for invalid input.
function parsePlayTime(text) {
    var t = (text || "").trim();
    if (t === "")
        return 0;
    var parts = t.split(":");
    if (parts.length > 3)
        return NaN;
    var total = 0;
    for (var i = 0; i < parts.length; i++) {
        var n = parseInt(parts[i], 10);
        if (isNaN(n) || n < 0)
            return NaN;
        if (i === parts.length - 1)
            total += n; // seconds
        else if (i === parts.length - 2)
            total += n * 60; // minutes
        else
            total += n * 3600; // hours
    }
    return total;
}
