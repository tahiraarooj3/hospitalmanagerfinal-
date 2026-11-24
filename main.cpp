// Hospital Management System - console interface
// Uses the existing PatientList, AppointmentScheduler and FileHandling classes.
#define _CRT_SECURE_NO_WARNINGS
#include "HospitalData.h"
#include "filehandelling.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <ctime>

using namespace std;

namespace UI {
    const bool useColor = (getenv("NO_COLOR") == nullptr);
    const string reset = useColor ? "\033[0m" : "";
    const string navy = useColor ? "\033[1;34m" : "";
    const string cyan = useColor ? "\033[1;36m" : "";
    const string green = useColor ? "\033[1;32m" : "";
    const string yellow = useColor ? "\033[1;33m" : "";
    const string red = useColor ? "\033[1;31m" : "";
    const string dim = useColor ? "\033[2m" : "";

    void clear() {
        // Works in macOS / VS Code interactive terminals; avoid escape codes in log files.
        const char* term = getenv("TERM");
        if (term && string(term) != "dumb") cout << "\033[2J\033[H";
        else cout << "\n";
    }

    string currentDate() {
        time_t now = time(nullptr);
        tm* local = localtime(&now);
        char buffer[80] = {0};
        if (local) strftime(buffer, sizeof(buffer), "%A, %d %B %Y  |  %I:%M %p", local);
        return buffer;
    }

    void header(const string& page) {
        cout << navy;
        cout << "+============================================================================+\n";
        cout << "|                      HOSPITAL MANAGEMENT SYSTEM                            |\n";
        cout << "|                  PATIENT & APPOINTMENT MANAGEMENT                          |\n";
        cout << "+============================================================================+\n";
        cout << reset;
        cout << "  " << cyan << page << reset << "\n";
        cout << "  " << dim << currentDate() << reset << "\n";
        cout << string(78, '-') << "\n\n";
    }

    void section(const string& title) {
        cout << "\n" << navy << "  [ " << title << " ]" << reset << "\n";
        cout << "  " << string(63, '-') << "\n";
    }

    void option(int number, const string& name) {
        cout << "  " << cyan << setw(2) << right << number << reset << "  |  " << name << "\n";
    }

    void dashboardStat(const string& label, const string& value) {
        cout << "  |  " << left << setw(25) << label
             << setw(27) << value << " |\n";
    }

    void success(const string& message) {
        cout << "\n" << green << "  [SUCCESS] " << reset << message << "\n";
    }

    void error(const string& message) {
        cout << "\n" << red << "  [ERROR] " << reset << message << "\n";
    }

    void info(const string& message) {
        cout << "\n" << yellow << "  [INFO] " << reset << message << "\n";
    }

    string trim(const string& text) {
        size_t start = text.find_first_not_of(" \t\r\n");
        if (start == string::npos) return "";
        size_t end = text.find_last_not_of(" \t\r\n");
        return text.substr(start, end - start + 1);
    }

    string line() {
        string value;
        if (!getline(cin, value)) throw runtime_error("Input stream closed");
        return trim(value);
    }

    void pause() {
        cout << "\n" << dim << "  Press ENTER to continue..." << reset;
        (void)line();
    }

    int number(const string& label, int minValue, int maxValue) {
        while (true) {
            cout << "  " << cyan << label << reset << ": ";
            string raw = line();
            istringstream input(raw);
            long long value;
            char extra;
            if ((input >> value) && !(input >> extra) &&
                value >= minValue && value <= maxValue) {
                return static_cast<int>(value);
            }
            error("Enter a whole number from " + to_string(minValue) +
                  " to " + to_string(maxValue) + ".");
        }
    }

    int numberOrKeep(const string& label, int oldValue, int minValue, int maxValue) {
        while (true) {
            cout << "  " << cyan << label << reset << " " << dim
                 << "[" << oldValue << "]" << reset << ": ";
            string raw = line();
            if (raw.empty()) return oldValue;
            istringstream input(raw);
            long long value;
            char extra;
            if ((input >> value) && !(input >> extra) &&
                value >= minValue && value <= maxValue) {
                return static_cast<int>(value);
            }
            error("Enter a whole number from " + to_string(minValue) +
                  " to " + to_string(maxValue) + ", or press ENTER to keep existing.");
        }
    }

    string text(const string& label, size_t maxLength = 45,
                const string& oldValue = "") {
        while (true) {
            cout << "  " << cyan << label << reset;
            if (!oldValue.empty()) cout << " " << dim << "[" << oldValue << "]" << reset;
            cout << ": ";
            string value = line();
            if (value.empty() && !oldValue.empty()) return oldValue;
            if (value.empty()) {
                error("This field cannot be empty.");
            } else if (value.size() > maxLength) {
                error("Maximum length: " + to_string(maxLength) + " characters.");
            } else if (value.find('|') != string::npos) {
                error("The | symbol cannot be used because records are saved to text files.");
            } else {
                return value;
            }
        }
    }

    bool confirm(const string& question) {
        while (true) {
            cout << "  " << yellow << question << " (y/n): " << reset;
            string answer = line();
            transform(answer.begin(), answer.end(), answer.begin(),
                      [](unsigned char c){ return static_cast<char>(tolower(c)); });
            if (answer == "y" || answer == "yes") return true;
            if (answer == "n" || answer == "no") return false;
            error("Type y for yes or n for no.");
        }
    }
}

static int patientCount(const PatientList& patients) {
    int count = 0;
    for (PatientNode* n = patients.head; n != nullptr; n = n->next) ++count;
    return count;
}

static string fit(const string& input, size_t width) {
    if (input.length() <= width) return input;
    if (width <= 3) return input.substr(0, width);
    return input.substr(0, width - 3) + "...";
}

static void tableLine() {
    cout << "  +--------+------------------+-----+--------+--------------+--------------+\n";
}

static void tableHeader() {
    tableLine();
    cout << "  | " << left << setw(6) << "ID"
         << " | " << setw(16) << "Patient Name"
         << " | " << setw(3) << "Age"
         << " | " << setw(6) << "Gender"
         << " | " << setw(12) << "Contact"
         << " | " << setw(12) << "Disease"
         << " |\n";
    tableLine();
}

static void patientRow(const Patient& p) {
    cout << "  | " << left << setw(6) << p.id
         << " | " << setw(16) << fit(p.name, 16)
         << " | " << setw(3) << p.age
         << " | " << setw(6) << fit(p.gender, 6)
         << " | " << setw(12) << fit(p.contact, 12)
         << " | " << setw(12) << fit(p.disease, 12)
         << " |\n";
}

static void tableFooter() {
    tableLine();
}

static void listPatients(const PatientList& patients) {
    if (!patients.head) {
        UI::info("No patient records found. Register a patient to get started.");
        return;
    }
    tableHeader();
    for (PatientNode* n = patients.head; n != nullptr; n = n->next)
        patientRow(n->data);
    tableFooter();
    cout << "\n  Total registered patients: " << patientCount(patients) << "\n";
}

static bool saveEverything(const PatientList& patients, const AppointmentScheduler& appointments) {
    bool patientsOK = FileHandling::savePatients(patients);
    bool appointmentsOK = FileHandling::saveAppointments(appointments);
    if (patientsOK && appointmentsOK)
        UI::success("Records saved to patients.txt and appointments.txt.");
    else
        UI::error("Could not save all records. Check file permissions and location.");
    return patientsOK && appointmentsOK;
}

static void registerPatient(PatientList& patients) {
    UI::section("REGISTER NEW PATIENT");
    int id = UI::number("Patient ID", 1, 1000000000);
    if (patients.searchbyid(id)) {
        UI::error("That Patient ID is already registered.");
        return;
    }
    string name = UI::text("Full name", 45);
    int age = UI::number("Age", 0, 130);
    string gender = UI::text("Gender", 20);
    string contact = UI::text("Contact number", 30);
    string disease = UI::text("Disease / reason for visit", 65);

    if (patients.registerPatient(Patient(id, name, age, gender, contact, disease))) {
        UI::success("Patient " + name + " (ID " + to_string(id) + ") registered.");
    } else {
        UI::error("Unable to register this patient.");
    }
}

static void searchPatient(PatientList& patients) {
    UI::section("SEARCH PATIENTS");
    cout << "  1  Search by patient ID\n";
    cout << "  2  Search by name (partial matches accepted)\n\n";
    int choice = UI::number("Choose", 1, 2);
    if (choice == 1) {
        int id = UI::number("Patient ID", 1, 1000000000);
        PatientNode* found = patients.searchbyid(id);
        if (!found) UI::error("Patient not found.");
        else { tableHeader(); patientRow(found->data); tableFooter(); }
    } else {
        string query = UI::text("Name to search", 45);
        transform(query.begin(), query.end(), query.begin(),
                  [](unsigned char c){ return static_cast<char>(tolower(c)); });
        int found = 0;
        for (PatientNode* n = patients.head; n != nullptr; n = n->next) {
            string name = n->data.name;
            transform(name.begin(), name.end(), name.begin(),
                      [](unsigned char c){ return static_cast<char>(tolower(c)); });
            if (name.find(query) != string::npos) {
                if (found == 0) tableHeader();
                patientRow(n->data);
                ++found;
            }
        }
        if (found) { tableFooter(); cout << "\n  " << found << " matching patient(s).\n"; }
        else UI::error("No matching patient found.");
    }
}

static void updatePatient(PatientList& patients) {
    UI::section("UPDATE PATIENT DETAILS");
    int id = UI::number("Patient ID", 1, 1000000000);
    PatientNode* found = patients.searchbyid(id);
    if (!found) { UI::error("Patient ID not found."); return; }

    cout << "\n  Current record:\n";
    tableHeader(); patientRow(found->data); tableFooter();
    UI::info("Press ENTER to keep a current text value.");
    string name = UI::text("Full name", 45, found->data.name);
    int age = UI::numberOrKeep("Age", found->data.age, 0, 130);
    string gender = UI::text("Gender", 20, found->data.gender);
    string contact = UI::text("Contact", 30, found->data.contact);
    string disease = UI::text("Disease / reason for visit", 65, found->data.disease);
    if (patients.updatePatient(id, name, age, gender, contact, disease))
        UI::success("Patient details updated.");
    else
        UI::error("Update failed.");
}

static void deletePatient(PatientList& patients, const AppointmentScheduler& appointments) {
    UI::section("DELETE PATIENT");
    int id = UI::number("Patient ID", 1, 1000000000);
    PatientNode* found = patients.searchbyid(id);
    if (!found) { UI::error("Patient ID not found."); return; }
    if (appointments.pendingappointmentexists(id)) {
        UI::error("This patient has a pending appointment. Cancel/complete it first.");
        return;
    }
    cout << "  Selected patient: " << found->data.name << "\n";
    if (!UI::confirm("Delete this patient permanently?")) {
        UI::info("Deletion cancelled.");
        return;
    }
    if (patients.deletePatient(id)) UI::success("Patient deleted.");
    else UI::error("Deletion failed.");
}

static void patientMenu(PatientList& patients, AppointmentScheduler& appointments) {
    while (true) {
        UI::clear(); UI::header("PATIENT MANAGEMENT");
        UI::option(1, "Register a new patient");
        UI::option(2, "View all patients");
        UI::option(3, "Search for a patient");
        UI::option(4, "Update patient details");
        UI::option(5, "Delete a patient");
        UI::option(6, "Sort patients by ID");
        UI::option(7, "Sort patients by name");
        cout << "\n  0  |  Return to main menu\n\n";
        int choice = UI::number("Enter your choice", 0, 7);
        if (choice == 0) return;
        UI::clear(); UI::header("PATIENT MANAGEMENT");
        switch (choice) {
            case 1: registerPatient(patients); break;
            case 2: UI::section("PATIENT DIRECTORY"); listPatients(patients); break;
            case 3: searchPatient(patients); break;
            case 4: updatePatient(patients); break;
            case 5: deletePatient(patients, appointments); break;
            case 6: patients.sortbyId(); UI::success("Patients sorted by ID."); listPatients(patients); break;
            case 7: patients.sortbyName(); UI::success("Patients sorted by name."); listPatients(patients); break;
        }
        UI::pause();
    }
}

static void bookAppointment(PatientList& patients, AppointmentScheduler& appointments) {
    UI::section("BOOK NEW APPOINTMENT");
    if (!patients.head) { UI::info("Register a patient first."); return; }
    int id = UI::number("Registered patient ID", 1, 1000000000);
    PatientNode* found = patients.searchbyid(id);
    if (!found) { UI::error("Patient not found. Register the patient before booking."); return; }
    cout << "  Patient: " << UI::green << found->data.name << UI::reset << "\n";
    string doctor = UI::text("Doctor name", 45);
    UI::info("Provide the appointment date and time (24-hour clock).");
    int day = UI::number("Day (DD)", 1, 31);
    int month = UI::number("Month (MM)", 1, 12);
    int year = UI::number("Year (YYYY)", 2024, 2100);
    int hour = UI::number("Hour (0-23)", 0, 23);
    int minute = UI::number("Minute (0-59)", 0, 59);
    cout << "\n";
    if (!appointments.bookAppointment(id, doctor, day, month, year, hour, minute, patients))
        UI::error("Booking was not added. Please check the details above.");
}

static void appointmentMenu(PatientList& patients, AppointmentScheduler& appointments) {
    while (true) {
        UI::clear(); UI::header("APPOINTMENT MANAGEMENT");
        UI::option(1, "Book an appointment");
        UI::option(2, "View pending appointments (FIFO)");
        UI::option(3, "Complete the next appointment");
        UI::option(4, "Cancel an appointment");
        UI::option(5, "View completed/cancelled history");
        cout << "\n  0  |  Return to main menu\n\n";
        int choice = UI::number("Enter your choice", 0, 5);
        if (choice == 0) return;
        UI::clear(); UI::header("APPOINTMENT MANAGEMENT");
        switch (choice) {
            case 1:
                bookAppointment(patients, appointments);
                break;
            case 2:
                UI::section("PENDING APPOINTMENTS");
                appointments.viewPendingQueue();
                break;
            case 3:
                UI::section("COMPLETE NEXT APPOINTMENT");
                if (UI::confirm("Mark the first pending appointment as completed?"))
                    appointments.completeNext();
                else UI::info("No changes made.");
                break;
            case 4: {
                UI::section("CANCEL APPOINTMENT");
                int id = UI::number("Appointment ID", 1, 1000000000);
                if (UI::confirm("Cancel appointment " + to_string(id) + "?"))
                    appointments.cancelAppointment(id);
                else UI::info("Cancellation aborted.");
                break;
            }
            case 5:
                UI::section("APPOINTMENT HISTORY");
                appointments.viewAppointmentHistory();
                break;
        }
        UI::pause();
    }
}

int main() {
    PatientList patients;
    AppointmentScheduler appointments;

    bool patientsLoaded = FileHandling::loadPatients(patients);
    bool appointmentsLoaded = FileHandling::loadAppointments(appointments);
    if (!patientsLoaded || !appointmentsLoaded) {
        UI::error("Some saved records could not be read correctly.");
        try { UI::pause(); } catch (...) {}
    }

    try {
        while (true) {
            UI::clear(); UI::header("MAIN DASHBOARD  /  LOCAL SYSTEM");
            cout << "  " << UI::navy << "+------------------- SYSTEM OVERVIEW -------------------+" << UI::reset << "\n";
            UI::dashboardStat("Registered patients", to_string(patientCount(patients)));
            UI::dashboardStat("Appointment tracking", "Available");
            UI::dashboardStat("Data storage", "Local text files");
            cout << "  " << UI::navy << "+-------------------------------------------------------+" << UI::reset << "\n";
            UI::section("MAIN MENU");
            UI::option(1, "Patient management");
            UI::option(2, "Appointment management");
            UI::option(3, "Save all records now");
            cout << "\n  " << UI::yellow << " 0" << UI::reset << "  |  Save and exit\n\n";

            int choice = UI::number("Select an option", 0, 3);
            if (choice == 1) patientMenu(patients, appointments);
            else if (choice == 2) appointmentMenu(patients, appointments);
            else if (choice == 3) {
                UI::section("SAVE RECORDS");
                saveEverything(patients, appointments);
                UI::pause();
            } else {
                UI::section("CLOSING THE SYSTEM");
                bool saved = saveEverything(patients, appointments);
                cout << "\n  Thank you for using Hospital Management System.\n\n";
                return saved ? 0 : 1;
            }
        }
    } catch (const runtime_error&) {
        cerr << "\nInput closed. Saving records before exit...\n";
        return saveEverything(patients, appointments) ? 0 : 1;
    }
}
