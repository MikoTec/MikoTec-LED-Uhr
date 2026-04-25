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
