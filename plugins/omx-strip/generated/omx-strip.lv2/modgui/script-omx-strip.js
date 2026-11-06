// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
// GENERATED — DO NOT EDIT BY HAND. Produced by tools/modgui-gen.mjs from
// plugins/omx-strip/omx-strip.decl.json (its panel block) and tools/look.json
// Regenerate: `node tools/modgui-gen.mjs`, then commit the result.
function (event, funcs) {
    var show = function (symbol, value) {
        var v = Math.round(value);
        event.icon.find('[data-visible-port="' + symbol + '"]').each(function () {
            // the mask is 64 bits of hex: the half that holds bit v, read as a plain number
            var hex = this.getAttribute('data-visible-mask');
            while (hex.length < 16) hex = '0' + hex;
            var word = parseInt(v < 32 ? hex.slice(8) : hex.slice(0, 8), 16);
            var on = v >= 0 && v <= 63 && Math.floor(word / Math.pow(2, v % 32)) % 2 === 1;
            this.style.display = on ? '' : 'none';
        });
    };
    if (event.type === 'start') {
        for (var i = 0; i < event.ports.length; i++) show(event.ports[i].symbol, event.ports[i].value);
    } else if (event.type === 'change') {
        show(event.symbol, event.value);
    }
}
