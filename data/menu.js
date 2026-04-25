// Menü-Overlay JavaScript

function toggleMenu() {
  document.getElementById('menuOverlay').classList.toggle('open');
}

function closeMenu() {
  document.getElementById('menuOverlay').classList.remove('open');
}

function rebootClock() {
  if (confirm('Uhr wirklich neu starten?')) {
    var x = new XMLHttpRequest();
    x.open('GET', '/reboot', true);
    x.send();
    alert('Uhr wird neu gestartet...');
  }
}

// Menü HTML dynamisch laden und einfügen
(function() {
  var menuHtml = '<button class="menu-btn" onclick="toggleMenu()" title="Menü">&#9776;</button>' +
    '<div class="menu-overlay" id="menuOverlay" onclick="if(event.target===this)closeMenu()">' +
    '<nav class="menu-panel">' +
    '<a href="/">Hauptseite</a>' +
    '<a href="/alarm">Alarm</a>' +
    '<hr class="menu-sep">' +
    '<a href="/settings">Einstellungen</a>' +
    '<div class="menu-sub">' +
    '<a href="/timezone">Zeitzone Manuell</a>' +
    '<a href="/update" class="menu-green">Firmware Update</a>' +
    '<a href="/update_fs" class="menu-green">Dateisystem Update</a>' +
    '</div>' +
    '<hr class="menu-sep">' +
    '<a href="/hilfe">Hilfe</a>' +
    '<div class="menu-sub">' +
    '<a href="/support">Support</a>' +
    '<a href="/cleareepromsure" class="menu-red">Werksreset</a>' +
    '<span class="menu-green" onclick="rebootClock()">Neustart</span>' +
    '</div>' +
    '</nav></div>';
  var ph = document.getElementById('menu-placeholder');
  if(ph) ph.innerHTML = menuHtml;
})();
