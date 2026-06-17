import { usePwaInstallPrompt } from "../hooks/pwa-install"
import { Dialog, DialogContent, DialogHeader, DialogTitle } from "@/components/ui/dialog"
import { Button } from "@/components/ui/button"
import { useState } from "react"

export function InstallPrompt() {
  const { isInstallable, promptInstall } = usePwaInstallPrompt()
  const [open, setOpen] = useState(true)

  if (!isInstallable) return null

  return (
    <Dialog open={open} onOpenChange={setOpen}>
      <DialogContent className="rounded-2xl shadow-lg">
        <DialogHeader>
          <DialogTitle>Install App</DialogTitle>
        </DialogHeader>
        <p>Would you like to install this app for quick access?</p>
        <div className="flex gap-2 mt-4">
          <Button onClick={promptInstall}>Yes, Install</Button>
          <Button variant="outline" onClick={() => setOpen(false)}>Maybe Later</Button>
        </div>
      </DialogContent>
    </Dialog>
  )
}
