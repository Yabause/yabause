//
//  FileSelectController.swift
//  uoyabause
//
//  Created by MiyamotoShinya on 2016/08/27.
//  Copyright © 2016年 devMiyax. All rights reserved.
//

import Foundation
import UIKit


class FileSelectController :UITableViewController {
    
    var file_list: [String] = [""]
    var selected_file_path: String = ""
    
    
    //required init?(coder aDecoder: NSCoder) {
    //    fatalError("init(coder:) has not been implemented")
    //}
    
    
    override func viewDidLoad(){
            super.viewDidLoad()
        
        file_list.removeAll()
        let manager = NSFileManager.defaultManager()
        let documentsPath = NSSearchPathForDirectoriesInDomains(.DocumentDirectory, .UserDomainMask, true)[0] as String
        
        do {
            let all_file_list = try manager.contentsOfDirectoryAtPath(documentsPath)
            for path in all_file_list {
                var isDir: ObjCBool = false
                if manager.fileExistsAtPath(documentsPath + "/" + path, isDirectory: &isDir) {
                    if( Bool(isDir) == false ){
                        file_list.append(path);
                    }
                } else {
                    
                }
            }
        }catch{
        }
    }
    
    override func didReceiveMemoryWarning() {
        super.didReceiveMemoryWarning()
    }
    
    override func tableView(tableView: UITableView, numberOfRowsInSection section: Int) -> Int{

        return file_list.count
    }
    
    override func tableView(tableView: UITableView, cellForRowAtIndexPath indexPath: NSIndexPath) -> UITableViewCell{
      
       
        var cell = tableView.dequeueReusableCellWithIdentifier("files")! as UITableViewCell
        cell.textLabel!.text = file_list[indexPath.row];
        return cell
    }
    
    override func tableView(table: UITableView, didSelectRowAtIndexPath indexPath:NSIndexPath) {
        
        let documentsPath = NSSearchPathForDirectoriesInDomains(.DocumentDirectory, .UserDomainMask, true)[0] as String
        
        selected_file_path = documentsPath + "/" + file_list[indexPath.row]
        performSegueWithIdentifier("toSubViewController",sender: nil)
        
    }
    
    // Segue 準備
    override func prepareForSegue(segue: UIStoryboardSegue, sender: AnyObject!) {
        if (segue.identifier == "toSubViewController") {
            let subVCmain: GameRevealViewController = (segue.destinationViewController as? GameRevealViewController)!
            subVCmain.selected_file = selected_file_path
        }
    }
    

}