//
//  SettingsViewController.swift
//  uoyabause
//
//  Created by MiyamotoShinya on 2016/09/25.
//  Copyright © 2016年 devMiyax. All rights reserved.
//

import Foundation
import UIKit


class SettingsViewController :UITableViewController,UIPickerViewDelegate, UIPickerViewDataSource {
 
    
    
    let _DATEPICKER_CELL_HEIGHT:CGFloat = 128.0
    
    let _cart_group = 0
    let _cart_index = 1
    var _CartPickerIsShowing = false
    private let cartArray: NSArray = ["None","4 Mbit BackupRam","8 Mbit BackupRam","16 Mbit BackupRam","32 Mbit BackupRam","8 Mbit DRAM","32 Mbit DRAM"]
    private let cartValues: NSArray = [ 0,2,3,4,5,6,7]
    @IBOutlet weak var _cart_sel_label: UILabel!
    @IBOutlet weak var _picker: UIPickerView!
   

    let _sound_group = 2
    let _sound_index = 0
    var _SoundPickerIsShowing = false
    private let soundArray: NSArray = ["High Quality but heavy","Low Quality but light"]
    private let soundValues: NSArray = [ 1,0 ]
    @IBOutlet weak var _sound_picker_label: UILabel!
    @IBOutlet weak var _soundPicker: UIPickerView!
    
    required init?(coder aDecoder: NSCoder) {
        super.init(coder: aDecoder)
    }
    
    @IBOutlet weak var _showFpsSwitch: UISwitch!
    @IBOutlet weak var _BultinBiosswitch: UISwitch!
    @IBOutlet weak var _showFrameSkip: UISwitch!
    @IBOutlet weak var _keepAspectRate: UISwitch!
    
    override func viewDidLoad() {
        super.viewDidLoad()
        _picker.hidden = !_CartPickerIsShowing
        _picker.delegate = self
        _picker.dataSource = self
        
        _soundPicker.hidden = !_SoundPickerIsShowing
        _soundPicker.delegate = self
        _soundPicker.dataSource = self
        

        //
        let plist = getSettingPlist()
        
        _BultinBiosswitch.on = plist.valueForKey("builtin bios") as! Bool
        _showFpsSwitch.on = plist.valueForKey("show fps") as! Bool
        _showFrameSkip.on = plist.valueForKey("frame skip") as! Bool
        _keepAspectRate.on = plist.valueForKey("keep aspect rate") as! Bool
        
        let cart_index = plist.valueForKey("cartridge") as! Int
        
        var index : Int = 0
        for  i in cartValues {
            if( cart_index == i as! Int){
                _cart_sel_label.text = cartArray[index] as! String
            }
            index += 1;
        }
       
        
        let sound_index = plist.valueForKey("sound engine") as! Int
        
        index = 0
        for  i in soundValues {
            if( sound_index == i as! Int){
                _sound_picker_label.text = soundArray[index] as! String
            }
            index += 1;
        }
        
    
    }
    
    func getSettingFilname() -> String {
        let libraryPath = NSSearchPathForDirectoriesInDomains(.LibraryDirectory, .UserDomainMask, true)[0] as! String
        let filename = "settings.plist"
        let filePath = libraryPath + "/" + filename
        
        return filePath
    }
    
    func getSettingPlist() -> NSMutableDictionary {
        let filePath = getSettingFilname()
        let manager = NSFileManager()
        if( !manager.fileExistsAtPath(filePath) ){
            let bundelfilePath = NSBundle.mainBundle().pathForResource("settings", ofType: "plist")
            do{
                
                try
                    NSFileManager.defaultManager().copyItemAtPath(bundelfilePath!, toPath: filePath)
                
            } catch let error as NSError {
                // Catch fires here, with an NSError being thrown
                print("error occurred, here are the details:\n \(error)")
            }
        }
        let plist = NSMutableDictionary(contentsOfFile:filePath)
        return plist!;
    }
    

   
    /*
     pickerに表示する列数を返すデータソースメソッド.
     (実装必須)
     */
    func numberOfComponentsInPickerView(pickerView: UIPickerView) -> Int {
        return 1
    }
    
    /*
     pickerに表示する行数を返すデータソースメソッド.
     (実装必須)
     */
    func pickerView(pickerView: UIPickerView, numberOfRowsInComponent component: Int) -> Int{
        
        if( pickerView == _picker){
            return cartArray.count
        }
        if( pickerView == _soundPicker){
            return soundArray.count
        }
        return 0;
        
        
    }
    
    /*
     pickerに表示する値を返すデリゲートメソッド.
     */
    func pickerView(pickerView: UIPickerView, titleForRow row: Int, forComponent component: Int) -> String? {
        
        if( pickerView == _picker){
            return cartArray[row] as? String
        }
 
        if( pickerView == _soundPicker){
            return soundArray[row] as? String
        }
        
        return soundArray[row] as? String
    }
    
    /*
     pickerが選択された際に呼ばれるデリゲートメソッド.
     */
    func pickerView(pickerView: UIPickerView, didSelectRow row: Int, inComponent component: Int) {
        
        if( pickerView == _picker ){
        
            let plist = getSettingPlist();
            plist.setObject(cartValues[row], forKey: "cartridge")
            plist.writeToFile(getSettingFilname(), atomically: true)
            
            _cart_sel_label.text = cartArray[row] as! String
       
            _CartPickerIsShowing = false;
            _picker.hidden = !_CartPickerIsShowing;
            self.tableView.beginUpdates()
            self.tableView.endUpdates()
        }
        
        if( pickerView == _soundPicker ){
            
            let plist = getSettingPlist();
            plist.setObject(soundValues[row], forKey: "sound engine")
            plist.writeToFile(getSettingFilname(), atomically: true)
            
            
            _sound_picker_label.text = soundArray[row] as! String
            
            _SoundPickerIsShowing = false;
            _soundPicker.hidden = !_SoundPickerIsShowing;
            self.tableView.beginUpdates()
            self.tableView.endUpdates()
        }
        
    }
    
    
    @IBAction func biosChanged(sender: AnyObject) {
        
        let plist = getSettingPlist();
        plist.setObject(_BultinBiosswitch.on, forKey: "builtin bios")
        plist.writeToFile(getSettingFilname(), atomically: true)
        
    }
    
    @IBAction func ShowFPSChanged(sender: AnyObject) {
       
        let plist = getSettingPlist();
        plist.setObject(_showFpsSwitch.on, forKey: "show fps")
        plist.writeToFile(getSettingFilname(), atomically: true)
    }
    
    @IBAction func FrameSkipChanged(sender: AnyObject) {
        
        let plist = getSettingPlist();
        plist.setObject(_showFrameSkip.on, forKey: "frame skip")
        plist.writeToFile(getSettingFilname(), atomically: true)
        

    }
    @IBAction func AspectrateChnaged(sender: AnyObject) {
        let plist = getSettingPlist();
        plist.setObject(_keepAspectRate.on, forKey: "keep aspect rate")
        plist.writeToFile(getSettingFilname(), atomically: true)
        
    }
    
    override func tableView(tableView: UITableView, didSelectRowAtIndexPath indexPath: NSIndexPath){
        
        if( indexPath.section == _cart_group && indexPath.row == _cart_index){
            _CartPickerIsShowing = !_CartPickerIsShowing;
            _picker.hidden = !_CartPickerIsShowing;
  
            self.tableView.beginUpdates()
            self.tableView.endUpdates()
            
            if( _picker.hidden == false ){
                
                self._picker.alpha = 0
                UIView.animateWithDuration(0.25, animations: { () -> Void in
                    self._picker.alpha = 1.0
                    }, completion: {(Bool) -> Void in
                   
                })
            }
        }
  
        if( indexPath.section == _sound_group && indexPath.row == _sound_index){
            _SoundPickerIsShowing = !_SoundPickerIsShowing;
            _soundPicker.hidden = !_SoundPickerIsShowing;
            
            self.tableView.beginUpdates()
            self.tableView.endUpdates()
            
            if( _soundPicker.hidden == false ){
                
                self._soundPicker.alpha = 0
                UIView.animateWithDuration(0.25, animations: { () -> Void in
                    self._soundPicker.alpha = 1.0
                    }, completion: {(Bool) -> Void in
                        
                })
            }
        }
    }
    
    
    override func tableView(tableView: UITableView, heightForRowAtIndexPath indexPath: NSIndexPath) -> CGFloat {
        var height: CGFloat = self.tableView.rowHeight
        
        if ( indexPath.section == _cart_group && indexPath.row == _cart_index+1){
            height =  self._CartPickerIsShowing ? self._DATEPICKER_CELL_HEIGHT : CGFloat(0)
        }

        if ( indexPath.section == _sound_group && indexPath.row == _sound_index+1){
            height =  self._SoundPickerIsShowing ? self._DATEPICKER_CELL_HEIGHT : CGFloat(0)
        }

        return height
    }
    
   
}
