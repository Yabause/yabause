//
//  MainScreenController.swift
//  uoyabause
//
//  Created by MiyamotoShinya on 2016/09/04.
//  Copyright © 2016年 devMiyax. All rights reserved.
//

import Foundation
import UIKit


class MainScreenController :UIViewController {

    @IBOutlet weak var menu_setting: UIBarButtonItem!
    var val: Int = 0
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        menu_setting.action = "onClickMyButton:"
        menu_setting.tag = 0
        
        val = 0
    }
    
    internal func onClickMyButton(sender: UIButton){
        let url = NSURL(string:UIApplicationOpenSettingsURLString)
        UIApplication.sharedApplication().openURL(url!)
    }
}