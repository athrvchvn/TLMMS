# Section 09 — Web Application Deployment

← [Firebase Deployment](08_firebase_deployment.md) | → [Card Issuing Station](10_card_issuing_station.md)

---

## Why This Section Exists

The web application is the admin interface for MMS V2. Admins use it to manage users, monitor machine status in real-time, view session logs, revoke cards, and trigger OTA updates. It is deployed to Firebase Hosting (a CDN) so it is accessible from any browser on any device.

---

## Prerequisites

- Node.js 18+ installed on your laptop
- Firebase CLI installed and logged in (see Section 08)
- Firebase project created with Authentication enabled
- `.env` file configured in the web app directory

---

## Step 1 — Configure Environment Variables

```bash
cd apps/web_app
cp .env.example .env
nano .env
```

Fill in every value from the Firebase Console:
```env
VITE_FIREBASE_API_KEY=AIzaSy...
VITE_FIREBASE_AUTH_DOMAIN=your-project.firebaseapp.com
VITE_FIREBASE_PROJECT_ID=your-project-id
VITE_FIREBASE_STORAGE_BUCKET=your-project.appspot.com
VITE_FIREBASE_MESSAGING_SENDER_ID=123456789
VITE_FIREBASE_APP_ID=1:123456789:web:abc123
VITE_FIREBASE_DATABASE_URL=https://your-project-default-rtdb.asia-southeast1.firebasedatabase.app
```

**How to find these values:**
- Firebase Console → Project Settings → General → "Your apps" section
- Click the web app (or add one if none exists)
- "SDK setup and configuration" → Config object
- Copy each value into the corresponding `.env` variable

---

## Step 2 — Build the Application

```bash
cd apps/web_app

# Install dependencies
npm install

# Build for production
npm run build

# The build output will be in: dist/
# Verify it was created:
ls dist/
# Should show: index.html, assets/, ...
```

---

## Step 3 — Deploy to Firebase Hosting

```bash
# From the repository root (where .firebaserc is)
cd /path/to/MMS/v2

firebase deploy --only hosting
# Expected output:
# ✔  Deploy complete!
# Hosting URL: https://your-project.web.app
```

---

## Step 4 — First Login and Admin Setup

The admin system uses two Firestore collections to track who is an admin:
- `/admins/{uid}` — regular admins (user management, machine control, session viewing)
- `/super_admins/{uid}` — super admins (OTA trigger, machine add/remove)

**Creating the first super admin:**

This must be done via Firebase Console (not the web app, because no admin exists yet):

1. **Create a Firebase Auth user:**
   - Firebase Console → Authentication → Users → Add user
   - Email: `admin@iiti.ac.in`
   - Password: (strong password)
   - Copy the UID shown after creation (format: `abc123xyz`)

2. **Add them to `super_admins` collection:**
   - Firestore Console → `super_admins` → Add document
   - Document ID: the UID from step above
   - Fields:
     ```
     email: "admin@iiti.ac.in"
     name: "Lab Coordinator"
     created_at: (use timestamp type, current time)
     is_active: true
     ```

3. **Also add to `admins` collection** (super admins need both):
   - Same document ID as UID
   - Same fields

4. **First login:**
   - Open `https://your-project.web.app`
   - Click "Admin Login"
   - Enter email and password
   - Should redirect to admin dashboard

---

## Step 5 — User Creation Procedure

Once logged in as admin:

1. Admin Panel → User Management → Add User
2. Fill in:
   - Roll Number (9 digits, e.g., `220002123`)
   - Name
   - Email
   - PIN (4 digits — stored as HMAC-SHA256 hash, never plaintext)
   - Machine permissions (checkboxes for each machine)
3. Click "Add User"
4. Firestore creates `/users/220002123` with all fields
5. Take the student to the kiosk to issue their physical card

---

## Step 6 — Adding Additional Admins

Once logged in as super admin:
1. Admin Panel → Super Admin Panel → Manage Admins → Add Admin
2. Enter email and name
3. This creates a Firebase Auth account + Firestore `/admins/{uid}` document

Or do it manually via Firebase Console (same as Step 4 but for `admins` collection only).

---

## Web App Feature Reference

| Feature | Location | Who Can Use |
|---|---|---|
| Real-time machine status | Dashboard | Admin, Super Admin |
| Session history / logs | Usage Logs | Admin, Super Admin |
| User management (add/edit) | User Management | Admin, Super Admin |
| Machine management | Machine Management | Admin, Super Admin |
| Remote stop session | Dashboard → machine card | Admin, Super Admin |
| Machine enable/disable | Machine Management | Admin, Super Admin |
| Card revocation | User Management → user → Revoke | Admin, Super Admin |
| OTA firmware update | Super Admin Panel | Super Admin only |
| Add/manage admins | Super Admin Panel | Super Admin only |

---

## Local Development

For development, run locally instead of deploying:
```bash
cd apps/web_app
npm run dev
# Opens at http://localhost:5173
```

This still connects to the real Firebase project. Use a separate development Firebase project if you want to test destructive operations without affecting production.

---

## Common Mistakes

| Mistake | Symptom | Fix |
|---|---|---|
| `.env` not filled in | App loads but shows Firebase error | Fill every `VITE_FIREBASE_*` variable |
| `VITE_FIREBASE_DATABASE_URL` missing | Real-time status doesn't update | Add RTDB URL to `.env` |
| Build not run before deploy | Old version of app deployed | Always run `npm run build` before `firebase deploy --only hosting` |
| Admin not in both `admins` and `super_admins` | Super admin features missing | Add to both collections with same UID |
| Wrong Firestore region | High latency on dashboards | Can't change after creation — recreate project if needed |
| No `admins` document created | Login works but no admin features | Create Firestore document manually as in Step 4 |
