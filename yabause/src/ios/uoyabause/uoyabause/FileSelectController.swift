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
        updateDoc()
    }
    
    func updateDoc(){
        file_list.removeAll()
        let manager = FileManager.default
        let documentsPath = NSSearchPathForDirectoriesInDomains(.documentDirectory, .userDomainMask, true)[0] as String
        
        do {
            let all_file_list = try manager.contentsOfDirectory(atPath: documentsPath)
            for path in all_file_list {
                var isDir: ObjCBool = false
                if manager.fileExists(atPath: documentsPath + "/" + path, isDirectory: &isDir) && !isDir.boolValue {
                
                        file_list.append(path);

                } else {
                    
                }
            }
            tableView.reloadData()
        }catch{
        }
    }
    
    override func didReceiveMemoryWarning() {
        super.didReceiveMemoryWarning()
    }
    
    override func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int{

        return file_list.count
    }
    
    override func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell{
      
       
        let cell = tableView.dequeueReusableCell(withIdentifier: "files")! as UITableViewCell
        cell.textLabel!.text = file_list[(indexPath as NSIndexPath).row];
        return cell
    }
    
    override func tableView(_ table: UITableView, didSelectRowAt indexPath:IndexPath) {
        
        let documentsPath = NSSearchPathForDirectoriesInDomains(.documentDirectory, .userDomainMask, true)[0] as String
        
        selected_file_path = documentsPath + "/" + file_list[(indexPath as NSIndexPath).row]
        performSegue(withIdentifier: "toSubViewController",sender: nil)
        
    }
    
    // Segue 準備
    override func prepare(for segue: UIStoryboardSegue, sender: Any!) {
        if (segue.identifier == "toSubViewController") {
            let subVCmain: GameRevealViewController = (segue.destination as? GameRevealViewController)!
            subVCmain.selected_file = selected_file_path
        }
    }
    

}
