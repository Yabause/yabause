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
    fileprivate let cartArray: NSArray = ["None","4 Mbit BackupRam","8 Mbit BackupRam","16 Mbit BackupRam","32 Mbit BackupRam","8 Mbit DRAM","32 Mbit DRAM"]
    fileprivate let cartValues: NSArray = [ 0,2,3,4,5,6,7]
    @IBOutlet weak var _cart_sel_label: UILabel!
    @IBOutlet weak var _picker: UIPickerView!
   
   
    let _sound_group = 2
    let _sound_index = 0
    var _SoundPickerIsShowing = false
    fileprivate let soundArray: NSArray = ["High Quality but heavy","Low Quality but light"]
    fileprivate let soundValues: NSArray = [ 1,0 ]
    @IBOutlet weak var _sound_picker_label: UILabel!
    @IBOutlet weak var _soundPicker: UIPickerView!

    let _resolution_group = 1
    let _resolution_index = 3
    var _ResolutionPickerIsShowing = false
    fileprivate let resolutionArray: NSArray = ["Native","4x","2x","Original"]
    fileprivate let resolutionValues: NSArray = [ 0,1,2,3]
    @IBOutlet weak var _resolution_sel_label: UILabel!
    @IBOutlet weak var _resolution_picker: UIPickerView!
    
    required init?(coder aDecoder: NSCoder) {
        super.init(coder: aDecoder)
    }
    
    @IBOutlet weak var _showFpsSwitch: UISwitch!
    @IBOutlet weak var _BultinBiosswitch: UISwitch!
    @IBOutlet weak var _showFrameSkip: UISwitch!
    @IBOutlet weak var _keepAspectRate: UISwitch!
    
    override func viewDidLoad() {
        super.viewDidLoad()
        _picker.isHidden = !_CartPickerIsShowing
        _picker.delegate = self
        _picker.dataSource = self
        
        _soundPicker.isHidden = !_SoundPickerIsShowing
        _soundPicker.delegate = self
        _soundPicker.dataSource = self
        
        _resolution_picker.isHidden = !_SoundPickerIsShowing
        _resolution_picker.delegate = self
        _resolution_picker.dataSource = self
        
        //
        let plist = getSettingPlist()
        
        _BultinBiosswitch.isOn = plist.value(forKey: "builtin bios") as! Bool
        _showFpsSwitch.isOn = plist.value(forKey: "show fps") as! Bool
        _showFrameSkip.isOn = plist.value(forKey: "frame skip") as! Bool
        _keepAspectRate.isOn = plist.value(forKey: "keep aspect rate") as! Bool
        
        let cart_index = plist.value(forKey: "cartridge") as! Int
        
        var index : Int = 0
        for  i in cartValues {
            if( cart_index == i as! Int){
                _cart_sel_label.text = cartArray[index] as! String
            }
            index += 1;
        }
       
        
        let sound_index = plist.value(forKey: "sound engine") as! Int
        
        index = 0
        for  i in soundValues {
            if( sound_index == i as! Int){
                _sound_picker_label.text = soundArray[index] as! String
            }
            index += 1;
        }
        
        var resolution_index = plist.value(forKey: "rendering resolution") as? Int
        if( resolution_index == nil ){
            resolution_index = 0
        }
        
        index = 0
        for  i in resolutionValues {
            if( resolution_index == i as! Int){
                _resolution_sel_label.text = resolutionArray[index] as! String
            }
            index += 1;
        }
    }
    
    func getSettingFilname() -> String {
        let libraryPath = NSSearchPathForDirectoriesInDomains(.libraryDirectory, .userDomainMask, true)[0] 
        let filename = "settings.plist"
        let filePath = libraryPath + "/" + filename
        
        return filePath
    }
    
    func getSettingPlist() -> NSMutableDictionary {
        let filePath = getSettingFilname()
        let manager = FileManager()
        if( !manager.fileExists(atPath: filePath) ){
            let bundelfilePath = Bundle.main.path(forResource: "settings", ofType: "plist")
            do{
                
                try
                    FileManager.default.copyItem(atPath: bundelfilePath!, toPath: filePath)
                
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
    func numberOfComponents(in pickerView: UIPickerView) -> Int {
        return 1
    }
    
    /*
     pickerに表示する行数を返すデータソースメソッド.
     (実装必須)
     */
    func pickerView(_ pickerView: UIPickerView, numberOfRowsInComponent component: Int) -> Int{
        
        if( pickerView == _picker){
            return cartArray.count
        }
        else if( pickerView == _soundPicker){
            return soundArray.count
        }
        else if( pickerView == _resolution_picker){
            return resolutionArray.count
        }
        return 0;
        
        
    }
    
    /*
     pickerに表示する値を返すデリゲートメソッド.
     */
    func pickerView(_ pickerView: UIPickerView, titleForRow row: Int, forComponent component: Int) -> String? {
        
        if( pickerView == _picker){
            return cartArray[row] as? String
        }
        else if( pickerView == _soundPicker){
            return soundArray[row] as? String
        }
        else if( pickerView == _resolution_picker){
            return resolutionArray[row] as? String
        }
        return soundArray[row] as? String
    }
    
    /*
     pickerが選択された際に呼ばれるデリゲートメソッド.
     */
    func pickerView(_ pickerView: UIPickerView, didSelectRow row: Int, inComponent component: Int) {
        
        if( pickerView == _picker ){
        
            let plist = getSettingPlist();
            plist.setObject(cartValues[row], forKey: "cartridge" as NSCopying)
            plist.write(toFile: getSettingFilname(), atomically: true)
            
            _cart_sel_label.text = cartArray[row] as! String
       
            _CartPickerIsShowing = false;
            _picker.isHidden = !_CartPickerIsShowing;
            self.tableView.beginUpdates()
            self.tableView.endUpdates()
        }
        
        else if( pickerView == _soundPicker ){
            
            let plist = getSettingPlist();
            plist.setObject(soundValues[row], forKey: "sound engine" as NSCopying)
            plist.write(toFile: getSettingFilname(), atomically: true)
            
            
            _sound_picker_label.text = soundArray[row] as! String
            
            _SoundPickerIsShowing = false;
            _soundPicker.isHidden = !_SoundPickerIsShowing;
            self.tableView.beginUpdates()
            self.tableView.endUpdates()
        }
        
        else if( pickerView == _resolution_picker ){
            
            let plist = getSettingPlist();
            plist.setObject(resolutionValues[row], forKey: "rendering resolution" as NSCopying)
            plist.write(toFile: getSettingFilname(), atomically: true)
            
            
            _resolution_sel_label.text = resolutionArray[row] as! String
            
            _ResolutionPickerIsShowing = false;
            _resolution_picker.isHidden = !_ResolutionPickerIsShowing;
            self.tableView.beginUpdates()
            self.tableView.endUpdates()
        }
        
    }
    
    
    @IBAction func biosChanged(_ sender: AnyObject) {
        
        let plist = getSettingPlist();
        plist.setObject(_BultinBiosswitch.isOn, forKey: "builtin bios" as NSCopying)
        plist.write(toFile: getSettingFilname(), atomically: true)
        
    }
    
    @IBAction func ShowFPSChanged(_ sender: AnyObject) {
       
        let plist = getSettingPlist();
        plist.setObject(_showFpsSwitch.isOn, forKey: "show fps" as NSCopying)
        plist.write(toFile: getSettingFilname(), atomically: true)
    }
    
    @IBAction func FrameSkipChanged(_ sender: AnyObject) {
        
        let plist = getSettingPlist();
        plist.setObject(_showFrameSkip.isOn, forKey: "frame skip" as NSCopying)
        plist.write(toFile: getSettingFilname(), atomically: true)
        

    }
    @IBAction func AspectrateChnaged(_ sender: AnyObject) {
        let plist = getSettingPlist();
        plist.setObject(_keepAspectRate.isOn, forKey: "keep aspect rate" as NSCopying)
        plist.write(toFile: getSettingFilname(), atomically: true)
        
    }
    
    override func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath){
        
        if( (indexPath as NSIndexPath).section == _cart_group && (indexPath as NSIndexPath).row == _cart_index){
            _CartPickerIsShowing = !_CartPickerIsShowing;
            _picker.isHidden = !_CartPickerIsShowing;
  
            self.tableView.beginUpdates()
            self.tableView.endUpdates()
            
            if( _picker.isHidden == false ){
                
                self._picker.alpha = 0
                UIView.animate(withDuration: 0.25, animations: { () -> Void in
                    self._picker.alpha = 1.0
                    }, completion: {(Bool) -> Void in
                   
                })
            }
        }
  
        if( (indexPath as NSIndexPath).section == _sound_group && (indexPath as NSIndexPath).row == _sound_index){
            _SoundPickerIsShowing = !_SoundPickerIsShowing;
            _soundPicker.isHidden = !_SoundPickerIsShowing;
            
            self.tableView.beginUpdates()
            self.tableView.endUpdates()
            
            if( _soundPicker.isHidden == false ){
                
                self._soundPicker.alpha = 0
                UIView.animate(withDuration: 0.25, animations: { () -> Void in
                    self._soundPicker.alpha = 1.0
                    }, completion: {(Bool) -> Void in
                        
                })
            }
        }
        
       
       
        if( (indexPath as NSIndexPath).section == _resolution_group && (indexPath as NSIndexPath).row == _resolution_index){
            _ResolutionPickerIsShowing = !_ResolutionPickerIsShowing;
            _resolution_picker.isHidden = !_ResolutionPickerIsShowing;
            
            self.tableView.beginUpdates()
            self.tableView.endUpdates()
            
            if( _resolution_picker.isHidden == false ){
                
                self._resolution_picker.alpha = 0
                UIView.animate(withDuration: 0.25, animations: { () -> Void in
                    self._resolution_picker.alpha = 1.0
                    }, completion: {(Bool) -> Void in
                        
                })
            }
        }
    }
    
    
    override func tableView(_ tableView: UITableView, heightForRowAt indexPath: IndexPath) -> CGFloat {
        var height: CGFloat = self.tableView.rowHeight
        
        if ( (indexPath as NSIndexPath).section == _cart_group && (indexPath as NSIndexPath).row == _cart_index+1){
            height =  self._CartPickerIsShowing ? self._DATEPICKER_CELL_HEIGHT : CGFloat(0)
        }

        if ( (indexPath as NSIndexPath).section == _sound_group && (indexPath as NSIndexPath).row == _sound_index+1){
            height =  self._SoundPickerIsShowing ? self._DATEPICKER_CELL_HEIGHT : CGFloat(0)
        }
        
        if ( (indexPath as NSIndexPath).section == _resolution_group && (indexPath as NSIndexPath).row == _resolution_index+1){
            height =  self._ResolutionPickerIsShowing ? self._DATEPICKER_CELL_HEIGHT : CGFloat(0)
        }
        
        return height
    }
    
   
}
