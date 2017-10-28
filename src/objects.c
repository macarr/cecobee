#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include ../include/utils.h

typedef struct Page {
    int page;
    int totalPages;
    int pageSize;
    int total;
}

typedef enum {registered, thermostats, managementSet} SelectionType;

typedef struct Selection {
    SelectionType selectionType;
    char[1000] selectionMatch;
    bool includeRuntime;
    bool includeExtendedRuntime;
    bool includeElectricity;
    bool includeSettings;
    bool includeLocation;
    bool includeProgram;
    bool includeEvents;
    bool includeDevice;
    bool includeTechnician;
    bool includeUtility;
    bool includeManagement;
    bool includeAlerts;
    bool includeReminders;
    bool includeWeather;
    bool includeHouseDetails;
    bool includeOemCfg;
    bool includeEquipmentStatus;
    bool includeNotificationSettings;
    bool includePrivacy;
    bool includeVersion;
    bool includeSecuritySettings;
    bool includeSensors;
    bool includeAudio;
} Selection;

typedef struct VoiceEngine {
    string name;
    bool enabled;
} VoiceEngine;

typedef struct Audio {
    int playbackVolume;
    bool microphoneEnabled;
    int soundAlertVolume;
    int soundTickVolume;
    VoiceEngine[] voiceEngines;
} Audio;

typedef struct Alert {
    string acknowledgeRef;
    string date; //YYYY-MM-DD
    string time;
    string severity;
    string text;
    int alertNumber;
    string alertType;
    bool isOperatorAlert;
    string reminder;
    bool showIdt;
    bool showWeb;
    bool sendEmail;
    string acknowledgement;
    bool remindMeLater;
    string thermostatIdentifier;
    string notificationType;
}

typedef struct Thermostat {
    string identifier;
    string name;
    string thermostatRev;
    bool isRegistered;
    string modelNumber;
    string brand;
    string features;
    string lastModified;
    string thermostatTime;
    string utcTime;
    Audio audio;
    Alert[] alerts;
}
