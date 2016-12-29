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

class MainScreenController :UIViewController {

//    @IBOutlet weak var menu_setting: UIBarButtonItem!
    @IBOutlet weak var bannerView: GADBannerView!
    
    var val: Int = 0
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        let filePath = Bundle.main.path(forResource: "apikey", ofType: "plist")
        let plist = NSDictionary(contentsOfFile:filePath!)
        let value = plist?.value(forKey: "ADMOB_KEY") as! String
        
        bannerView.adUnitID = value
        
        bannerView.rootViewController = self
        let request = GADRequest()
        //request.testDevices = ["ea16d8d48e439597ec9e49ec690fe356"]
        bannerView.load(request)
        //bannerView.hidden = true
        
//        menu_setting.action = "onClickMyButton:"
//        menu_setting.tag = 0
        
        val = 0
    }
    
    internal func onClickMyButton(_ sender: UIButton){
        let url = URL(string:UIApplicationOpenSettingsURLString)
        UIApplication.shared.openURL(url!)
    }
}
