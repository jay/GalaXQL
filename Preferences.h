#ifndef PREFERENCES_H_
#define PREFERENCES_H_

#include <wx/string.h>


/* Create the prefs table if it does not exist.
*/
void CreatePrefsTableIfNotExists();

/* Get the value of a preference.
If the value is not found return 'default_value'.
*/
wxString GetPreference(const wxString &name, const wxString &default_value);
int GetPreference(const wxString &name, const int default_value);

/* Set the value of a preference.
*/
void SetPreference(const wxString &name, const wxString &value);
void SetPreference(const wxString &name, const int value);

/* Delete a preference.
*/
void DeletePreference(const wxString &name);

/* Read all preferences and apply them to the application.
If a preference does not exist use its default value.
*/
void RestorePreferences();

/* Recreate the prefs table and apply the default preferences to the
application.
*/
void ResetPreferences();

#endif // PREFERENCES_H_
