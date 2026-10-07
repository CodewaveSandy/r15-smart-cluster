import QtQuick

Canvas {
    id: icon
    required property string kind
    property color tint: "white"
    width: 28; height: 28
    onKindChanged: requestPaint()
    onTintChanged: requestPaint()
    onWidthChanged: requestPaint()
    onHeightChanged: requestPaint()
    onPaint: {
        const c = getContext("2d")
        c.reset(); c.scale(width / 32, height / 32)
        c.strokeStyle = tint; c.fillStyle = tint; c.lineWidth = 2
        c.lineCap = "round"; c.lineJoin = "round"
        function path(points, filled) {
            c.beginPath(); c.moveTo(points[0][0], points[0][1])
            for (let i = 1; i < points.length; ++i) c.lineTo(points[i][0], points[i][1])
            if (filled) { c.closePath(); c.fill() } else c.stroke()
        }
        function circle(x, y, r) { c.beginPath(); c.arc(x, y, r, 0, Math.PI * 2); c.stroke() }
        switch (kind) {
        case "left": case "right":
            if (kind === "right") { c.translate(32, 0); c.scale(-1, 1) }
            path([[3,16],[14,5],[14,11],[29,11],[29,21],[14,21],[14,27]], true); break
        case "beam":
            path([[14,6],[22,6],[28,11],[28,21],[22,26],[14,26],[14,6]], false)
            for (let y = 7; y < 27; y += 5) path([[3,y],[10,y]], false)
            break
        case "engine":
            path([[3,13],[8,13],[8,9],[14,9],[14,6],[21,6],[21,9],[25,9],[25,13],[29,13],[29,24],[24,24],[21,27],[9,27],[6,23],[3,23],[3,13]], false)
            path([[2,16],[2,21]], false); break
        case "temp":
            path([[15,4],[15,19]], false); circle(15,21,3)
            for (let y = 6; y < 17; y += 4) path([[16,y],[21,y]], false)
            for (let y of [26,30]) path([[2,y],[6,y-2],[10,y],[14,y-2],[18,y],[22,y-2],[26,y],[30,y-2]], false)
            break
        case "hazard":
            path([[16,3],[30,28],[2,28],[16,3]], false)
            path([[16,10],[24,24],[8,24],[16,10]], false)
            path([[16,14],[16,18]], false); circle(16,21,0.6); break
        case "message":
            path([[3,5],[29,5],[29,23],[14,23],[7,29],[7,23],[3,23]], true)
            c.strokeStyle = "#14202c"; path([[9,11],[23,11]], false); path([[9,16],[19,16]], false); break
        case "call":
            path([[8,3],[3,7],[4,15],[10,24],[18,29],[25,29],[29,24],[23,19],[19,23],[13,18],[9,12],[12,8]], true); break
        case "phone":
            c.strokeRect(9,2,14,28); path([[13,5],[19,5]], false); circle(16,26,0.8); break
        case "fuel":
            c.strokeRect(5,3,14,25); c.strokeRect(8,6,8,8)
            path([[3,29],[21,29]], false)
            path([[19,15],[23,15],[23,25],[27,27],[29,25],[29,10],[25,6]], false)
            path([[25,6],[25,12],[29,14]], false); break
        case "abs":
            circle(16,16,10)
            c.beginPath(); c.arc(16,16,14,Math.PI*0.65,Math.PI*1.35); c.stroke()
            c.beginPath(); c.arc(16,16,14,-Math.PI*0.35,Math.PI*0.35); c.stroke()
            c.font = "bold 9px sans-serif"; c.textAlign = "center"; c.fillText("ABS",16,19); break
        case "tcs": case "neutral":
            c.font = "bold " + (kind === "neutral" ? "25" : "13") + "px sans-serif"
            c.textAlign = "center"; c.fillText(kind === "neutral" ? "N" : "TCS",16,24); break
        }
    }
}
