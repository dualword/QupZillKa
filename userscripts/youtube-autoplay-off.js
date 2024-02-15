// ==UserScript==
// @name          youtube-autoplay-off
// @description   Turns autoplay button off
// @match         https://www.youtube.com/watch*
// @author        https://github.com/dualword/QupZillKa/
// @namespace     https://github.com/dualword/QupZillKa/
// @version       0.1
// @license       MIT
// ==/UserScript==

var id = 'button[title="Autoplay is on"]';
t=window.setInterval(test, 5000);

function test(){
	var o = document.querySelector(id);
	o.click();
	window.clearInterval(t)
}
