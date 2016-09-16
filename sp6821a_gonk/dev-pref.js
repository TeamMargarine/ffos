
pref("layers.composer2d.enabled", false);

pref("dom.ipc.processPriorityManager.backgroundLRUPoolLevels", 2);

// Enable Out Of Background Process killer.
// Number of background process would have a hard limit according to the
// calculation result from backgroundLRUPoolLevels argument.
pref("hal.processPriorityManager.gonk.enableOOBPKiller", true);

// Geolocation configuration to use the Mozilla Location Service.
pref("geo.provider.use_mls", true);
pref("geo.wifi.uri", "https://location.services.mozilla.com/v1/geolocate?key=%MOZ_MOZILLA_API_KEY%");
pref("geo.cell.scan", true);
