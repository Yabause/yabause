//
//  MainScreenController.swift
//  uoyabause
//
//  Created by MiyamotoShinya on 2016/09/04.
//  Copyright © 2016年 devMiyax. All rights reserved.
//

import Foundation
import UIKit
import GoogleMobileAds

class MainScreenController :UIViewController, UIDocumentPickerDelegate, GADBannerViewDelegate {

//    @IBOutlet weak var menu_setting: UIBarButtonItem!
    @IBOutlet weak var bannerView: GADBannerView!
    
    @IBAction func onAddFile(_ sender: Any) {
        let dv = UIDocumentPickerViewController(documentTypes:  ["public.item"], in: .import)
        dv.delegate = self
        //dv.allowsDocumentCreation = false
        //dv.allowsPickingMultipleItems = false
        self.present(dv, animated:true, completion: nil)
    }
    
    func documentPicker(_ controller: UIDocumentPickerViewController, didPickDocumentsAt documentURLs: [URL]){
        print(documentURLs[0])
        
        var documentsUrl = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask)[0]
        
        let theFileName = documentURLs[0].lastPathComponent
        
        if !theFileName.lowercased().contains(".chd") {
            let alert: UIAlertController = UIAlertController(title: "Fail to open", message: "You can open a CHD only", preferredStyle:  UIAlertController.Style.alert)
            
            let defaultAction: UIAlertAction = UIAlertAction(title: "OK", style: UIAlertAction.Style.default, handler:{
                (action: UIAlertAction!) -> Void in
                print("OK")
            })
            
            alert.addAction(defaultAction)
            
            present(alert, animated: true, completion: nil)
            return
        }

        documentsUrl.appendPathComponent(theFileName)
        let fileManager = FileManager.default
         do {
            try fileManager.copyItem(at: documentURLs[0], to: documentsUrl)
         } catch let error as NSError {
            NSLog("Fail to copy \(error.localizedDescription)")
         }
        
        self.children.forEach{
            //if ($0.isKind(of: FileSelectController)){
            let fc = $0 as? FileSelectController
            if fc != nil {
                fc?.updateDoc()
            }
            //}
        }
       
    }
    var val: Int = 0
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        let filePath = Bundle.main.path(forResource: "apikey", ofType: "plist")
        let plist = NSDictionary(contentsOfFile:filePath!)
        let value = plist?.value(forKey: "ADMOB_KEY") as! String
        
        bannerView.adUnitID = value
        
        bannerView.rootViewController = self
        bannerView.delegate = self
        
        let request = GADRequest()
        
        //request.testDevices = ["ea16d8d48e439597ec9e49ec690fe356"]
        bannerView.load(request)
        //bannerView.hidden = true
        
//        menu_setting.action = "onClickMyButton:"
//        menu_setting.tag = 0
        
        val = 0
    }
    
    internal func onClickMyButton(_ sender: UIButton){
        let url = URL(string:UIApplication.openSettingsURLString)
        UIApplication.shared.openURL(url!)
    }
    
    /// Tells the delegate an ad request loaded an ad.
    func adViewDidReceiveAd(_ bannerView: GADBannerView) {
      print("adViewDidReceiveAd")
    }

    /// Tells the delegate an ad request failed.
    func adView(_ bannerView: GADBannerView,
        didFailToReceiveAdWithError error: GADRequestError) {
      print("adView:didFailToReceiveAdWithError: \(error.localizedDescription)")
    }

    /// Tells the delegate that a full-screen view will be presented in response
    /// to the user clicking on an ad.
    func adViewWillPresentScreen(_ bannerView: GADBannerView) {
      print("adViewWillPresentScreen")
    }

    /// Tells the delegate that the full-screen view will be dismissed.
    func adViewWillDismissScreen(_ bannerView: GADBannerView) {
      print("adViewWillDismissScreen")
    }

    /// Tells the delegate that the full-screen view has been dismissed.
    func adViewDidDismissScreen(_ bannerView: GADBannerView) {
      print("adViewDidDismissScreen")
    }

    /// Tells the delegate that a user click will open another app (such as
    /// the App Store), backgrounding the current app.
    func adViewWillLeaveApplication(_ bannerView: GADBannerView) {
      print("adViewWillLeaveApplication")
    }
    
}
