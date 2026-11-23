.pragma library
// Built-in backgrounds. A background is stored as "g:<id>" (gradient) or a file URL.
var gradients = [
    { id: "sunset",   name: "Zonsopgang", stops: ["#2a2a5e", "#9a4f8f", "#f2a35e"] },
    { id: "ocean",    name: "Oceaan",     stops: ["#06223d", "#0f5c8a", "#4fb3d9"] },
    { id: "night",    name: "Nacht",      stops: ["#05060d", "#141a3a", "#2b2f66"] },
    { id: "forest",   name: "Bos",        stops: ["#0b2418", "#1f5a3a", "#7fb77e"] },
    { id: "dawn",     name: "Ochtend",    stops: ["#3a1c4a", "#c2577a", "#ffd29a"] },
    { id: "graphite", name: "Grafiet",    stops: ["#101114", "#23262d", "#3a3e48"] }
]
function isGradient(b) { return !b || b.indexOf("g:") === 0 }
function stops(b) {
    var id = b ? b.substring(2) : "sunset"
    for (var i = 0; i < gradients.length; ++i)
        if (gradients[i].id === id) return gradients[i].stops
    return gradients[0].stops
}
